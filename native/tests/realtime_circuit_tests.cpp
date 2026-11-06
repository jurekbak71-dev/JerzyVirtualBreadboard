#include "realtime_circuit.hpp"
#include "circuit.hpp"
#include <cmath>
#include <iostream>
#include <chrono>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error("CHECK failed: " #x);}while(0)
int main(){
 using namespace vbal;
 constexpr double sr=48000.,freq=1000.;
 auto rc=makeRcCircuit(10000.,1e-8,false);
 RealtimeCircuitProcessor p(rc,sr);
 CHECK(p.unknownNodeCount()==1);
 double inRms=0,outRms=0;int count=0;
 for(int i=0;i<sr;i++){
  float x=float(std::sin(2.*3.141592653589793*freq*i/sr));
  float y=p.process(x);
  if(i>24000){inRms+=x*x;outRms+=y*y;++count;}
 }
 const double gain=std::sqrt(outRms/inRms);
 const double expected=1./std::sqrt(1.+std::pow(2.*3.141592653589793*freq*10000.*1e-8,2));
 CHECK(std::abs(gain-expected)<0.04);
 auto clip=makeRcCircuit(10000.,1e-8,true);
 RealtimeCircuitProcessor d(clip,sr);
 double peak=0.;
 for(int i=0;i<24000;i++){
  float x=float(2.0*std::sin(2.*3.141592653589793*400.*i/sr));
  peak=std::max(peak,std::abs(double(d.process(x))));
 }
 CHECK(peak<1.3);
 CHECK(peak>0.1);
 Circuit two;
 two.parts.push_back({"R1",PartType::Resistor,"in","mid",1000.});
 two.parts.push_back({"R2",PartType::Resistor,"mid","out",2000.});
 two.parts.push_back({"C1",PartType::Capacitor,"out","0",1e-7});
 RealtimeCircuitProcessor q(two,sr);
 CHECK(q.unknownNodeCount()==2);
 float last=0;for(int i=0;i<4000;i++)last=q.process(i>50?1.f:0.f);
 CHECK(last>0.9f&&last<1.01f);
 Circuit rl;
 rl.parts.push_back({"R1",PartType::Resistor,"in","out",1000.});
 rl.parts.push_back({"L1",PartType::Inductor,"out","0",0.01});
 RealtimeCircuitProcessor rlp(rl,sr);
 float rlLast=0.f;for(int i=0;i<8000;i++)rlLast=rlp.process(i>10?1.f:0.f);
 CHECK(std::isfinite(rlLast));
 Circuit ledCircuit;
 ledCircuit.parts.push_back({"R1",PartType::Resistor,"in","out",1000.});
 ledCircuit.parts.push_back({"DLED1",PartType::Led,"out","0",0.});
 RealtimeCircuitProcessor ledp(ledCircuit,sr);
 double ledPositive=0.,ledNegative=0.;for(int i=0;i<12000;i++){const float x=float(4.*std::sin(2.*3.141592653589793*300.*i/sr));const double y=ledp.process(x);ledPositive=std::max(ledPositive,y);ledNegative=std::min(ledNegative,y);}
 CHECK(ledPositive<3.5);
 CHECK(ledPositive>0.5);
 CHECK(ledNegative<-3.0);
 auto diodePeak=[&](PartType type,float amplitude){
  Circuit c;c.parts.push_back({"R1",PartType::Resistor,"in","out",1000.});
  const char* ref=type==PartType::SchottkyDiode?"DSCH1":type==PartType::GermaniumDiode?"DGE1":"DZ1";
  c.parts.push_back({ref,type,"out","0",0.});
  RealtimeCircuitProcessor proc(c,sr);double pos=0.,neg=0.;
  for(int i=0;i<12000;i++){const double y=proc.process(float(amplitude*std::sin(2.*3.141592653589793*250.*i/sr)));pos=std::max(pos,y);neg=std::min(neg,y);}
  return std::pair<double,double>{pos,neg};
 };
 const auto sch=diodePeak(PartType::SchottkyDiode,2.f);
 const auto ge=diodePeak(PartType::GermaniumDiode,2.f);
 CHECK(sch.first<1.0);CHECK(ge.first<1.0);
 const auto z=diodePeak(PartType::ZenerDiode,7.f);
 CHECK(z.second>-6.5);
 CHECK(z.second<-3.5);
 Circuit swClosed;
 swClosed.parts.push_back({"RSW1",PartType::Switch,"in","out",1.});
 swClosed.parts.push_back({"R1",PartType::Resistor,"out","0",1000.});
 RealtimeCircuitProcessor swOn(swClosed,sr);
 float on=0.f;for(int i=0;i<200;i++)on=swOn.process(1.f);
 CHECK(on>0.95f);
 Circuit swOpen=swClosed;swOpen.parts[0].value=0.;
 RealtimeCircuitProcessor swOff(swOpen,sr);
 float off=0.f;for(int i=0;i<200;i++)off=swOff.process(1.f);
 CHECK(std::abs(off)<0.001f);
 bool singular=false;
 try{
  Circuit bad;bad.parts.push_back({"R1",PartType::Resistor,"in","out",1000.});
  bad.parts.push_back({"C1",PartType::Capacitor,"floating","0",1e-7});
  RealtimeCircuitProcessor fail(bad,sr);
 }catch(const std::exception&){singular=true;}
 CHECK(singular);
 auto perfCircuit=makeRcCircuit(10000.,1e-8,true);
 RealtimeCircuitProcessor perf(perfCircuit,48000.);
 const auto begin=std::chrono::steady_clock::now();
 double checksum=0.;
 for(int i=0;i<48000*2;i++)checksum+=perf.process(float(std::sin(2.*3.141592653589793*997.*i/48000.)));
 const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
 CHECK(std::isfinite(checksum));
 CHECK(elapsed<2.0);
 std::cout<<"PASS: realtime R/C/L/D/LED solver gain, clipping, state, multi-node and invalid topology; 2 s audio in "<<elapsed<<" s\n";
}
