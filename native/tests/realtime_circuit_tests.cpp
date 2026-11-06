#include "realtime_circuit.hpp"
#include "circuit.hpp"
#include <cmath>
#include <iostream>
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
 bool singular=false;
 try{
  Circuit bad;bad.parts.push_back({"R1",PartType::Resistor,"in","out",1000.});
  bad.parts.push_back({"C1",PartType::Capacitor,"floating","0",1e-7});
  RealtimeCircuitProcessor fail(bad,sr);
 }catch(const std::exception&){singular=true;}
 CHECK(singular);
 std::cout<<"PASS: realtime R/C/D solver gain, clipping, multi-node and invalid topology\n";
}
