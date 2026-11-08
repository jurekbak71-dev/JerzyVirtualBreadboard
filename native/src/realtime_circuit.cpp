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
RealtimeCircuitProcessor::RealtimeCircuitProcessor(const Circuit& circuit,double sampleRate,const LiveResistorControls* liveResistors)
 :circuit_(circuit),sampleRate_(sampleRate){
 validateCircuit(circuit_);
 if(!std::isfinite(sampleRate_)||sampleRate_<8000.||sampleRate_>384000.)
  throw std::invalid_argument("Invalid realtime sample rate");
 std::unordered_set<std::string> nodes;
 std::unordered_map<std::string,int> fixedCodes;
 for(const auto& p:circuit_.parts){
  nodes.insert(p.pin1);nodes.insert(p.pin2);
  if(p.type==PartType::NpnTransistor||p.type==PartType::PnpTransistor||p.type==PartType::NmosTransistor||p.type==PartType::NjfetTransistor)nodes.insert(p.pin3);
  if(p.type==PartType::OpAmp||p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101){
   nodes.insert(p.pin3);nodes.insert(p.pin4);nodes.insert(p.pin5);
   for(const auto& pin:p.extraPins)nodes.insert(pin);
  }
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
 fixedCodes_=fixedCodes;
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
 for(const auto& p:circuit_.parts)if(p.type!=PartType::DcVoltageSource){
  const bool three=p.type==PartType::NpnTransistor||p.type==PartType::PnpTransistor||p.type==PartType::NmosTransistor||p.type==PartType::NjfetTransistor;
  parts_.push_back({p,idx(p.pin1),idx(p.pin2),(three||p.type==PartType::OpAmp||p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101)?idx(p.pin3):kGround,
   (p.type==PartType::OpAmp||p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101)?idx(p.pin4):kGround,
   (p.type==PartType::OpAmp||p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101)?idx(p.pin5):kGround,0.,0.});
  auto& rp=parts_.back();
  if(p.type==PartType::Resistor){
   rp.smoothResistance=p.value;
   if(liveResistors){
    const auto found=liveResistors->find(p.reference);
    if(found!=liveResistors->end())rp.liveResistance=found->second;
   }
  }
  if(p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101){
   rp.icPins={idx(p.pin1),idx(p.pin2),idx(p.pin3),idx(p.pin4),idx(p.pin5)};
   for(size_t i=0;i<p.extraPins.size();++i)rp.icPins[i+5]=idx(p.extraPins[i]);
   if(p.type==PartType::Pt2399){
   rp.pt2399=std::make_unique<Pt2399Core>(sampleRate_);
   const auto& vco=p.extraPins[0];
   const auto& digitalGround=p.pin4;
   for(const auto& candidate:circuit_.parts){
    if(candidate.type==PartType::Resistor&&
      ((candidate.pin1==vco&&candidate.pin2==digitalGround)||(candidate.pin2==vco&&candidate.pin1==digitalGround))){
      rp.pt2399ControlOhms=candidate.value;
      if(liveResistors){
       const auto it=liveResistors->find(candidate.reference);
       if(it!=liveResistors->end())rp.liveClockResistance=it->second;
      }
      break;
    }
   }
   if(!(rp.pt2399ControlOhms>0.))
    throw std::invalid_argument("PT2399 pin 6 needs a physical control resistor to pin 4 (DGND)");
   }else if(p.type==PartType::Mn3007){
    rp.bbd=std::make_unique<Mn3007Core>(sampleRate_);
   }else if(p.type==PartType::Mn3101){
    // Panasonic MN3101 RC oscillator: R2 at pin 6 and C1 at pin 5 to GND.
    // fCP approximated from published oscillator curves; external parts
    // and supply are represented by actual circuit connections.
    double r=0.,c=0.;
    for(const auto& part:circuit_.parts){
     if(part.type==PartType::Resistor&&
       ((part.pin1==p.extraPins[0]&&part.pin2==p.pin1)||(part.pin2==p.extraPins[0]&&part.pin1==p.pin1)))
      {
       r=part.value;
       if(liveResistors){
        const auto it=liveResistors->find(part.reference);
        if(it!=liveResistors->end())rp.liveClockResistance=it->second;
       }
      }
     if(part.type==PartType::Capacitor&&
       ((part.pin1==p.pin5&&part.pin2==p.pin1)||(part.pin2==p.pin5&&part.pin1==p.pin1)))
      c=part.value;
    }
    if(!(r>0.&&c>0.))throw std::invalid_argument("MN3101 needs physical R between OX2 pin 6 and ground pin 1 and C between OX3 pin 5 and ground pin 1");
    rp.clockCapacitance=c;
    rp.clockHz=1./(12.*r*c);
    if(rp.clockHz<10000.||rp.clockHz>100000.)throw std::invalid_argument("MN3101 external RC produces clock outside MN3007 10-100kHz limit");
   }
  }
 }
 // A BBD must have both complementary clock pins joined to a real MN3101
 // component, not a hardcoded chorus macro.
 for(auto& device:parts_)if(device.part.type==PartType::Mn3007){
  bool driven=false;
  for(const auto& driver:parts_)if(driver.part.type==PartType::Mn3101&&
    device.icPins[1]==driver.icPins[1]&&device.icPins[5]==driver.icPins[3]){
   device.clockHz=driver.clockHz;
   device.clockCapacitance=driver.clockCapacitance;
   device.liveClockResistance=driver.liveClockResistance;
   driven=true;break;
  }
  if(!driven)throw std::invalid_argument("MN3007 needs MN3101 CP1/CP2 connected to physical pins 2 and 6");
 }
 outputIndex_=idx(circuit_.outputNode);
 if(outputIndex_<0)throw std::invalid_argument("Realtime output must be a solved node");
 const auto n=unknowns_.size();matrix_.resize(n*n);rhs_.resize(n);solution_.resize(n);guess_.resize(n);
 reset();
}
void RealtimeCircuitProcessor::reset(){
 for(auto& p:parts_){p.previousVoltage=0.;p.previousCurrent=0.;p.pt2399Voltage=0.;if(p.pt2399)p.pt2399->reset();p.bbdOutput=0.;p.clockPhase=0.;if(p.bbd)p.bbd->reset();}
 std::fill(solution_.begin(),solution_.end(),0.);
 std::fill(guess_.begin(),guess_.end(),0.);
}
double RealtimeCircuitProcessor::fixedVoltage(int index,float input) const noexcept{
 if(index==kInput)return double(input);
 if(index<=-3){const size_t i=size_t(-3-index);return i<fixedVoltages_.size()?fixedVoltages_[i]:0.;}
 return 0.;
}
int RealtimeCircuitProcessor::probeCode(const std::string& node) const noexcept{
 if(node==circuit_.groundNode)return -1;
 if(node==circuit_.inputNode)return -2;
 if(auto it=fixedCodes_.find(node);it!=fixedCodes_.end())return it->second;
 if(auto it=nodeIndex_.find(node);it!=nodeIndex_.end())return it->second;
 return -1000000;
}
float RealtimeCircuitProcessor::voltageAtCode(int code,float input) const noexcept{
 if(code==-1000000)return 0.f;
 if(code>=0&&size_t(code)<solution_.size())return float(solution_[size_t(code)]);
 return float(fixedVoltage(code,input));
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
 // Compute a stable per-sample resistance before Newton iterations. Atomic
 // controls are written by the GUI thread; no live graph mutation/allocations.
 const double smoothAlpha=1.-std::exp(-1./(sampleRate_*0.004));
 for(auto& part:parts_){
  if(part.liveResistance){
   const double target=part.liveResistance->load(std::memory_order_relaxed);
   if(std::isfinite(target)&&target>=0.01&&target<=1e9)
    part.smoothResistance+=smoothAlpha*(target-part.smoothResistance);
  }
  if(part.liveClockResistance){
   const double resistance=part.liveClockResistance->load(std::memory_order_relaxed);
   if(std::isfinite(resistance)&&resistance>=0.01&&resistance<=1e9){
    if(part.part.type==PartType::Pt2399)
     part.pt2399ControlOhms+=smoothAlpha*(resistance-part.pt2399ControlOhms);
    else if(part.clockCapacitance>0.){
     const double hz=1./(12.*resistance*part.clockCapacitance);
     if(hz>=10000.&&hz<=100000.)
      part.clockHz+=smoothAlpha*(hz-part.clockHz);
    }
   }
  }
 }
 const size_t n=unknowns_.size();
 for(int iteration=0;iteration<8;++iteration){
  std::fill(matrix_.begin(),matrix_.end(),0.);
  std::fill(rhs_.begin(),rhs_.end(),0.);
  for(const auto& rp:parts_){
   const auto& p=rp.part;
   if(p.type==PartType::Mn3101){
    const auto v=[&](int node){return node>=0?guess_[size_t(node)]:fixedVoltage(node,input);};
    const auto& pin=rp.icPins;
    const double ground=v(pin[0]),supply=v(pin[2]);
    const bool powered=supply-ground<=-8.&&supply-ground>=-16.2;
    const double clockA=powered?(rp.clockPhase<0.5?ground:supply):ground;
    const double clockB=powered?(rp.clockPhase<0.5?supply:ground):ground;
    const double gate=powered?supply+(ground-supply)/15.:ground;
    for(const auto& entry:std::array<std::pair<int,double>,3>{{{pin[1],clockA},{pin[3],clockB},{pin[7],gate}}}){
     stampConductance(entry.first,pin[0],0.02,input);
     stampCurrent(entry.first,pin[0],-0.02*(entry.second-ground));
    }
    for(int k:{4,5,6})stampConductance(pin[k],pin[0],1e-9,input);
   }else if(p.type==PartType::Mn3007){
    const auto v=[&](int node){return node>=0?guess_[size_t(node)]:fixedVoltage(node,input);};
    const auto& pin=rp.icPins;
    const double ground=v(pin[0]),supply=v(pin[4]),vgg=v(pin[3]);
    const bool powered=(supply-ground<=-13.8&&supply-ground>=-16.2&&
      std::abs((vgg-supply)-1.)<1.5);
    for(int k:{6,7}){
     const double tap=k==6?rp.bbd->output1():rp.bbd->output2();
     const double output=powered?(ground+(supply-ground)*0.5+tap):ground;
     stampConductance(pin[k],pin[0],0.02,input);
     stampCurrent(pin[k],pin[0],-0.02*(output-ground));
    }
   }else if(p.type==PartType::Pt2399){
    // Reduced-pin, supply-aware mixed-signal macro-model:
    // p1 VCC, p2 REF, p3 AGND, p4 DGND, p6 control R,
    // p14 LPF2-OUT, p15 LPF1-OUT, p16 LPF1-IN.
    // The isolated PT2399Core models internal timed digital memory,
    // while surrounding R/C circuitry is solved by this physical MNA.
    const auto v=[&](int node){return node>=0?guess_[size_t(node)]:fixedVoltage(node,input);};
    const auto& pin=rp.icPins;
    const double vcc=v(pin[0])-v(pin[2]);
    const double groundOffset=v(pin[2]);
    const bool powered=(vcc>=4.5&&vcc<=5.5)&&std::abs(v(pin[3])-groundOffset)<0.15;
    // Instrumented internal reference divider at pin 2.
    stampConductance(pin[1],pin[2],1./3000.,input);
    stampCurrent(pin[1],pin[2],powered?-(vcc*.5)/3000.:0.);
    // The analog input front-end output at pin 15 is an approximate buffer.
    const double frontEnd=powered?v(pin[15])-groundOffset:0.;
    stampConductance(pin[14],pin[2],0.02,input);
    stampCurrent(pin[14],pin[2],-0.02*frontEnd);
    // Pin 14 is driven by the already clocked internal digital delay.
    stampConductance(pin[13],pin[2],0.02,input);
    stampCurrent(pin[13],pin[2],powered?-0.02*(vcc*.5+rp.pt2399Voltage):0.);
    // Other peripheral pins have only an ultraweak leakage path. This is
    // explicitly not a transistor-level PT2399 simulation.
    for(const int k:{4,5,6,7,8,9,10,11,12})
     stampConductance(pin[k],pin[2],1e-9,input);
   }else if(p.type==PartType::Resistor){
    stampConductance(rp.a,rp.b,1./(rp.liveResistance?std::max(0.01,rp.smoothResistance):p.value),input);
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
    constexpr double is=1e-15,vt=0.025852;
    const double beta=p.value;
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
   }else if(p.type==PartType::PnpTransistor){
    const auto voltage=[&](int index){return index>=0?guess_[size_t(index)]:fixedVoltage(index,input);};
    const double vb=voltage(rp.b),ve=voltage(rp.c);
    const double veb=std::clamp(ve-vb,-1.0,0.9);
    const double beta=p.value,vt=0.025852;
    const double is=p.germanium?5e-6/beta:1e-15;
    const double expv=std::exp(std::clamp(veb/vt,-40.,30.));
    const double ieb=is*(expv-1.);
    const double geb=std::max(is/vt*expv,1e-12);
    const double ieq=ieb-geb*veb;
    stampConductance(rp.c,rp.b,geb,input);
    stampCurrent(rp.c,rp.b,ieq);
    const double gm=beta*geb,iceq=beta*ieq;
    auto addCtl=[&](int row,int node,double coeff){
     if(row<0)return;
     if(node>=0)matrix_[size_t(row)*n+size_t(node)]+=coeff;
     else rhs_[size_t(row)]-=coeff*fixedVoltage(node,input);
    };
    addCtl(rp.c,rp.c,gm);addCtl(rp.c,rp.b,-gm);
    addCtl(rp.a,rp.c,-gm);addCtl(rp.a,rp.b,gm);
    stampCurrent(rp.c,rp.a,iceq);
    stampConductance(rp.a,rp.c,p.germanium?2e-5:1e-6,input);
   }else if(p.type==PartType::NmosTransistor){
    const auto voltage=[&](int index){return index>=0?guess_[size_t(index)]:fixedVoltage(index,input);};
    const double vd=voltage(rp.a),vg=voltage(rp.b),vs=voltage(rp.c);
    const double vgs=vg-vs,vds=vd-vs;
    constexpr double kp=0.02,lambda=0.02;
    const double vth=p.value;
    double id=0.,gm=0.,gds=1e-9;
    if(vgs>vth){
     const double over=vgs-vth;
     if(vds<over){
      id=kp*(over*vds-0.5*vds*vds)*(1.+lambda*std::max(vds,0.0));
      gm=kp*vds*(1.+lambda*std::max(vds,0.0));
      gds=std::max(kp*(over-vds)*(1.+lambda*std::max(vds,0.0))+lambda*std::max(id,0.0),1e-9);
     }else{
      id=0.5*kp*over*over*(1.+lambda*std::max(vds,0.0));
      gm=kp*over*(1.+lambda*std::max(vds,0.0));
      gds=std::max(0.5*kp*over*over*lambda,1e-9);
     }
    }
    const double ieq=id-gm*vgs-gds*vds;
    stampConductance(rp.a,rp.c,gds,input);
    auto addCtl=[&](int row,int node,double coeff){
     if(row<0)return;
     if(node>=0)matrix_[size_t(row)*n+size_t(node)]+=coeff;
     else rhs_[size_t(row)]-=coeff*fixedVoltage(node,input);
    };
    addCtl(rp.a,rp.b,gm);addCtl(rp.a,rp.c,-gm);
    addCtl(rp.c,rp.b,-gm);addCtl(rp.c,rp.c,gm);
    stampCurrent(rp.a,rp.c,ieq);
   }else if(p.type==PartType::OpAmp){
    const auto voltage=[&](int index){return index>=0?guess_[size_t(index)]:fixedVoltage(index,input);};
    const double vp=voltage(rp.b),vm=voltage(rp.c),vhi=voltage(rp.d),vlo=voltage(rp.e);
    const double high=std::max(vhi,vlo)-0.2,low=std::min(vhi,vlo)+0.2;
    const double gain=std::clamp(p.value,10.,1000000.);
    const double diff=vp-vm;
    const double voGuess=voltage(rp.a);
    constexpr double gout=1.0;
    stampConductance(rp.a,kGround,gout,input);
    const bool satHigh=voGuess>high||(voGuess>=high-1e-6&&diff>0.);
    const bool satLow=voGuess<low||(voGuess<=low+1e-6&&diff<0.);
    if(satHigh){
     stampCurrent(rp.a,kGround,-gout*high);
    }else if(satLow){
     stampCurrent(rp.a,kGround,-gout*low);
    }else{
     auto addCtl=[&](int row,int node,double coeff){
      if(row<0)return;
      if(node>=0)matrix_[size_t(row)*n+size_t(node)]+=coeff;
      else rhs_[size_t(row)]-=coeff*fixedVoltage(node,input);
     };
     addCtl(rp.a,rp.b,-gout*gain);
     addCtl(rp.a,rp.c,gout*gain);
    }
   }else if(p.type==PartType::NjfetTransistor){
    const auto voltage=[&](int index){return index>=0?guess_[size_t(index)]:fixedVoltage(index,input);};
    const double vd=voltage(rp.a),vg=voltage(rp.b),vs=voltage(rp.c);
    const double vgs=vg-vs,vds=vd-vs;
    constexpr double beta=0.002,lambda=0.02;
    const double vto=-p.value;
    double id=0.,gm=0.,gds=1e-9;
    if(vgs>vto){
     const double over=vgs-vto;
     if(vds<over){
      id=beta*(over*vds-0.5*vds*vds)*(1.+lambda*std::max(vds,0.0));
      gm=beta*vds*(1.+lambda*std::max(vds,0.0));
      gds=std::max(beta*(over-vds)*(1.+lambda*std::max(vds,0.0))+lambda*std::max(id,0.0),1e-9);
     }else{
      id=0.5*beta*over*over*(1.+lambda*std::max(vds,0.0));
      gm=beta*over*(1.+lambda*std::max(vds,0.0));
      gds=std::max(0.5*beta*over*over*lambda,1e-9);
     }
    }
    const double ieq=id-gm*vgs-gds*vds;
    stampConductance(rp.a,rp.c,gds,input);
    auto addCtl=[&](int row,int node,double coeff){
     if(row<0)return;
     if(node>=0)matrix_[size_t(row)*n+size_t(node)]+=coeff;
     else rhs_[size_t(row)]-=coeff*fixedVoltage(node,input);
    };
    addCtl(rp.a,rp.b,gm);addCtl(rp.a,rp.c,-gm);
    addCtl(rp.c,rp.b,-gm);addCtl(rp.c,rp.c,gm);
    stampCurrent(rp.a,rp.c,ieq);
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
  else if(rp.part.type==PartType::Mn3101){
   rp.clockPhase+=rp.clockHz/sampleRate_;
   rp.clockPhase-=std::floor(rp.clockPhase);
  }else if(rp.part.type==PartType::Mn3007){
   const auto& pin=rp.icPins;
   const double ground=voltage(pin[0]),vdd=voltage(pin[4]),vgg=voltage(pin[3]);
   // MN3007 input and bias are both relative to its local GND (pin 1).
   // Using the absolute input voltage incorrectly clips a translated -15 V
   // supply domain (for example GND=+5 V, VDD=-10 V).
   const float signal=float(voltage(pin[2])-ground);
   // Reference each bucket to the actual power-rail midpoint, not an invented fixed -7.5 V.
   // External bias resistors still set the physical input node and can cause clipping.
   const double midSupply=ground+0.5*(vdd-ground);
   rp.bbdOutput=rp.bbd->step(signal,rp.clockHz,vdd-ground,vgg-ground,midSupply-ground);
  }else if(rp.part.type==PartType::Pt2399){
   const auto& pin=rp.icPins;
   const double vcc=voltage(pin[0])-voltage(pin[2]);
   const double groundDelta=voltage(pin[3])-voltage(pin[2]);
   // Internal converter input is referred to the physical REF/virtual ground.
   const float acIn=float(voltage(pin[15])-voltage(pin[1]));
   rp.pt2399Voltage=rp.pt2399->step(acIn,rp.pt2399ControlOhms,std::abs(groundDelta)<0.15?vcc:0.);
  }
 }
 const double out=solution_[size_t(outputIndex_)];
 return float(std::clamp(out,-4.,4.));
}
}
