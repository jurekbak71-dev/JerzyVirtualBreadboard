#include "realtime_circuit.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
namespace vbal {
namespace {
constexpr int kGround=-1;
constexpr int kInput=-2;
constexpr double kDiodeIs=2.52e-9;
constexpr double kDiodeN=1.906;
constexpr double kThermalVoltage=0.025852;
}
RealtimeCircuitProcessor::RealtimeCircuitProcessor(const Circuit& circuit,double sampleRate)
 :circuit_(circuit),sampleRate_(sampleRate){
 validateCircuit(circuit_);
 if(!std::isfinite(sampleRate_)||sampleRate_<8000.||sampleRate_>384000.)
  throw std::invalid_argument("Invalid realtime sample rate");
 std::unordered_set<std::string> nodes;
 std::unordered_map<std::string,int> fixedCodes;
 for(const auto& p:circuit_.parts){
  nodes.insert(p.pin1);nodes.insert(p.pin2);if(p.type==PartType::NpnTransistor)nodes.insert(p.pin3);
  if(p.type==PartType::DcVoltageSource){
   const bool p1g=p.pin1==circuit_.groundNode,p2g=p.pin2==circuit_.groundNode;
   if(p1g==p2g)throw std::invalid_argument("Realtime DC source must connect one terminal to ground");
   const std::string node=p1g?p.pin2:p.pin1;
   if(node==circuit_.inputNode||node==circuit_.outputNode||fixedCodes.contains(node))throw std::invalid_argument("Invalid or duplicate fixed DC node");
   const double voltage=p1g?-p.value:p.value;
   fixedVoltages_.push_back(voltage);
   fixedCodes[node]=-3-static_cast<int>(fixedVoltages_.size()-1);
  }
 }
 nodes.erase(circuit_.groundNode);nodes.erase(circuit_.inputNode);
 for(const auto& [node,code]:fixedCodes)nodes.erase(node);
 if(nodes.empty()||nodes.size()>64)throw std::invalid_argument("Realtime circuit requires 1..64 solved nodes");
 unknowns_.assign(nodes.begin(),nodes.end());
 std::sort(unknowns_.begin(),unknowns_.end());
 for(size_t i=0;i<unknowns_.size();++i)nodeIndex_[unknowns_[i]]=static_cast<int>(i);
 auto idx=[&](const std::string& n){
  if(n==circuit_.groundNode)return kGround;
  if(n==circuit_.inputNode)return kInput;
  if(auto it=fixedCodes.find(n);it!=fixedCodes.end())return it->second;
  auto it=nodeIndex_.find(n);if(it==nodeIndex_.end())throw std::invalid_argument("Unknown realtime node");
  return it->second;
 };
 for(const auto& p:circuit_.parts)if(p.type!=PartType::DcVoltageSource)parts_.push_back({p,idx(p.pin1),idx(p.pin2),p.type==PartType::NpnTransistor?idx(p.pin3):kGround,0.,0.});
 outputIndex_=idx(circuit_.outputNode);
 if(outputIndex_<0)throw std::invalid_argument("Realtime output must be a solved node");
 const auto n=unknowns_.size();matrix_.resize(n*n);rhs_.resize(n);solution_.resize(n);guess_.resize(n);
 reset();
}
void RealtimeCircuitProcessor::reset(){
 for(auto& p:parts_){p.previousVoltage=0.;p.previousCurrent=0.;}
 std::fill(solution_.begin(),solution_.end(),0.);
 std::fill(guess_.begin(),guess_.end(),0.);
}
double RealtimeCircuitProcessor::fixedVoltage(int index,float input) const noexcept{
 if(index==kInput)return double(input);
 if(index<=-3){const size_t i=size_t(-3-index);return i<fixedVoltages_.size()?fixedVoltages_[i]:0.;}
 return 0.;
}
void RealtimeCircuitProcessor::stampConductance(int a,int b,double g,float input){
 auto add=[&](int r,int c,double v){if(r>=0&&c>=0)matrix_[size_t(r)*unknowns_.size()+size_t(c)]+=v;};
 if(a>=0){
  add(a,a,g);
  if(b>=0)add(a,b,-g);else rhs_[size_t(a)]+=g*fixedVoltage(b,input);
 }
 if(b>=0){
  add(b,b,g);
  if(a>=0)add(b,a,-g);else rhs_[size_t(b)]+=g*fixedVoltage(a,input);
 }
}
void RealtimeCircuitProcessor::stampCurrent(int a,int b,double current){
 if(a>=0)rhs_[size_t(a)]-=current;
 if(b>=0)rhs_[size_t(b)]+=current;
}
bool RealtimeCircuitProcessor::solveLinear(){
 const size_t n=unknowns_.size();
 for(size_t i=0;i<n;++i){
  size_t pivot=i;double best=std::abs(matrix_[i*n+i]);
  for(size_t r=i+1;r<n;++r){const auto v=std::abs(matrix_[r*n+i]);if(v>best){best=v;pivot=r;}}
  if(best<1e-18||!std::isfinite(best))return false;
  if(pivot!=i){
   for(size_t c=i;c<n;++c)std::swap(matrix_[i*n+c],matrix_[pivot*n+c]);
   std::swap(rhs_[i],rhs_[pivot]);
  }
  const double diag=matrix_[i*n+i];
  for(size_t r=i+1;r<n;++r){
   const double factor=matrix_[r*n+i]/diag;if(factor==0.)continue;
   matrix_[r*n+i]=0.;
   for(size_t c=i+1;c<n;++c)matrix_[r*n+c]-=factor*matrix_[i*n+c];
   rhs_[r]-=factor*rhs_[i];
  }
 }
 for(size_t ri=n;ri-->0;){
  double value=rhs_[ri];
  for(size_t c=ri+1;c<n;++c)value-=matrix_[ri*n+c]*solution_[c];
  solution_[ri]=value/matrix_[ri*n+ri];
  if(!std::isfinite(solution_[ri]))return false;
 }
 return true;
}
float RealtimeCircuitProcessor::process(float input){
 if(!std::isfinite(input))input=0.f;
 input=std::clamp(input,-4.f,4.f);
 guess_=solution_;
 const size_t n=unknowns_.size();
 for(int iteration=0;iteration<8;++iteration){
  std::fill(matrix_.begin(),matrix_.end(),0.);
  std::fill(rhs_.begin(),rhs_.end(),0.);
  for(const auto& rp:parts_){
   const auto& p=rp.part;
   if(p.type==PartType::Resistor){
    stampConductance(rp.a,rp.b,1./p.value,input);
   }else if(p.type==PartType::Switch){
    stampConductance(rp.a,rp.b,p.value>0.5?100.:1e-12,input);
   }else if(p.type==PartType::Capacitor){
    const double g=p.value*sampleRate_;
    stampConductance(rp.a,rp.b,g,input);
    stampCurrent(rp.a,rp.b,-g*rp.previousVoltage);
   }else if(p.type==PartType::Inductor){
    const double g=1./(p.value*sampleRate_);
    stampConductance(rp.a,rp.b,g,input);
    stampCurrent(rp.a,rp.b,rp.previousCurrent);
   }else if(p.type==PartType::NpnTransistor){
    const auto voltage=[&](int index){return index>=0?guess_[size_t(index)]:fixedVoltage(index,input);};
    const double vb=voltage(rp.b),ve=voltage(rp.c);
    const double vbe=std::clamp(vb-ve,-1.0,0.9);
    constexpr double is=1e-15,vt=0.025852,beta=100.;
    const double expv=std::exp(std::clamp(vbe/vt,-40.,30.));
    const double ibe=is*(expv-1.);
    const double gbe=std::max(is/vt*expv,1e-12);
    const double ieq=ibe-gbe*vbe;
    stampConductance(rp.b,rp.c,gbe,input);
    stampCurrent(rp.b,rp.c,ieq);
    const double gm=beta*gbe,iceq=beta*ieq;
    auto addCtl=[&](int row,int node,double coeff){
     if(row<0)return;
     if(node>=0)matrix_[size_t(row)*n+size_t(node)]+=coeff;
     else rhs_[size_t(row)]-=coeff*fixedVoltage(node,input);
    };
    addCtl(rp.a,rp.b,gm);addCtl(rp.a,rp.c,-gm);
    addCtl(rp.c,rp.b,-gm);addCtl(rp.c,rp.c,gm);
    stampCurrent(rp.a,rp.c,iceq);
    stampConductance(rp.a,rp.c,1e-6,input);
   }else{
    const auto voltage=[&](int index){return index>=0?guess_[size_t(index)]:fixedVoltage(index,input);};
    const bool led=p.type==PartType::Led;
    const bool sch=p.type==PartType::SchottkyDiode;
    const bool ge=p.type==PartType::GermaniumDiode;
    const bool zener=p.type==PartType::ZenerDiode;
    const double is=led?1e-20:sch?1e-7:ge?1e-6:kDiodeIs;
    const double nFactor=led?2.0:sch?1.05:ge?1.1:kDiodeN;
    const double minV=zener?-7.0:(led?-3.0:-1.5),maxV=led?4.0:(sch||ge?1.0:1.5);
    const double vd=std::clamp(voltage(rp.a)-voltage(rp.b),minV,maxV);
    const double scale=nFactor*kThermalVoltage;
    const double expv=std::exp(std::clamp(vd/scale,-40.,led?50.:30.));
    double current=is*(expv-1.);
    double g=std::max(is/scale*expv,1e-12);
    if(zener&&vd<-4.8){
     constexpr double bv=5.1,ibv=1e-3,zscale=0.08;
     const double ez=std::exp(std::clamp((-vd-bv)/zscale,-20.,20.));
     current-=ibv*ez;
     g+=ibv/zscale*ez;
    }
    const double ieq=current-g*vd;
    stampConductance(rp.a,rp.b,g,input);
    stampCurrent(rp.a,rp.b,ieq);
   }
  }
  if(!solveLinear())throw std::runtime_error("Realtime circuit matrix is singular");
  double maxDelta=0.;for(size_t i=0;i<n;++i){maxDelta=std::max(maxDelta,std::abs(solution_[i]-guess_[i]));guess_[i]=solution_[i];}
  if(maxDelta<1e-7)break;
 }
 auto voltage=[&](int index){return index>=0?solution_[size_t(index)]:fixedVoltage(index,input);};
 for(auto& rp:parts_){
  const double v=voltage(rp.a)-voltage(rp.b);
  if(rp.part.type==PartType::Capacitor)rp.previousVoltage=v;
  else if(rp.part.type==PartType::Inductor)rp.previousCurrent+=v/(rp.part.value*sampleRate_);
 }
 const double out=solution_[size_t(outputIndex_)];
 return float(std::clamp(out,-4.,4.));
}
}
