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
 CHECK(p.probeCode("in")==-2);
 CHECK(p.probeCode("0")==-1);
 CHECK(p.probeCode("out")>=0);
 CHECK(p.probeCode("not_wired")==-1000000);
 const float probedOutput=p.voltageAtCode(p.probeCode("out"),0.f);
 CHECK(std::isfinite(probedOutput));
 CHECK(std::abs(probedOutput)<=4.f);
 const float inputSample=0.37f;
 CHECK(std::abs(p.voltageAtCode(p.probeCode("in"),inputSample)-inputSample)<1e-6f);
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
 Circuit biased;
 biased.parts.push_back({"V1",PartType::DcVoltageSource,"vcc","0",1.});
 biased.parts.push_back({"RIN",PartType::Resistor,"in","out",1000000000.});
 biased.parts.push_back({"RUP",PartType::Resistor,"vcc","out",1000.});
 biased.parts.push_back({"RDN",PartType::Resistor,"out","0",1000.});
 RealtimeCircuitProcessor bias(biased,sr);
 float biasOut=0.f;for(int i=0;i<100;i++)biasOut=bias.process(0.f);
 CHECK(biasOut>0.49f&&biasOut<0.51f);
 Circuit amp;
 amp.parts.push_back({"V1",PartType::DcVoltageSource,"vcc","0",9.});
 amp.parts.push_back({"RC",PartType::Resistor,"vcc","out",4700.});
 amp.parts.push_back({"Q1",PartType::NpnTransistor,"out","in",100.,"0"});
 RealtimeCircuitProcessor ampProc(amp,sr);
 float q0=0.f;for(int i=0;i<200;i++)q0=ampProc.process(0.60f);
 float q1=0.f;for(int i=0;i<200;i++)q1=ampProc.process(0.70f);
 CHECK(std::isfinite(q0)&&std::isfinite(q1));
 CHECK(q0>q1);
 CHECK(std::abs(q1-q0)>0.1f);
 Circuit pnp;
 pnp.parts.push_back({"V1",PartType::DcVoltageSource,"vcc","0",1.});
 pnp.parts.push_back({"RC",PartType::Resistor,"out","0",4700.});
 pnp.parts.push_back({"QP1",PartType::PnpTransistor,"out","in",100.,"vcc"});
 RealtimeCircuitProcessor pnpProc(pnp,sr);
 float pnpWeak=0.f;for(int i=0;i<200;i++)pnpWeak=pnpProc.process(0.55f);
 float pnpStrong=0.f;for(int i=0;i<200;i++)pnpStrong=pnpProc.process(0.50f);
 CHECK(std::isfinite(pnpWeak)&&std::isfinite(pnpStrong));
 CHECK(pnpStrong>pnpWeak);
 CHECK(std::abs(pnpStrong-pnpWeak)>0.01f);


 // Two genuinely germanium PNP devices with negative supply and external RC network.
 // Verify that the realtime MNA engine, not a named DSP macro, responds to supply changes.
 const auto makeGeFuzz=[&](int supply){
  const std::string deck=
   "V1 vneg 0 "+std::to_string(supply)+"\n"
   "C1 in b1 2.2e-6\n"
   "Q1 q1c b1 0 GPNP_Q1\n"
   "R1 q1c vneg 33000\n"
   "Q2 q2c q1c q2e GPNP_Q2\n"
   "R2 q2e 0 470\n"
   "R3 q2e 0 1000\n"
   "C2 q2e 0 20e-6\n"
   "R4 q2c vneg 8200\n"
   "R5 q2c b1 100000\n"
   "C3 q2c out 10e-9\n"
   "R6 out 0 500000\n"
   ".model GPNP_Q1 PNP(Is=5e-6 Bf=85 Vaf=40 Eg=0.72)\n"
   ".model GPNP_Q2 PNP(Is=5e-6 Bf=120 Vaf=40 Eg=0.72)\n";
  return parseEditorSpiceElements(deck);
 };
 RealtimeCircuitProcessor fuzz9(makeGeFuzz(-9),sr),fuzz6(makeGeFuzz(-6),sr);
 double fuzzDifference=0.,fuzzEnergy=0.;
 for(int i=0;i<12000;++i){
  const float x=float(0.5*std::sin(2.*3.141592653589793*330.*i/sr));
  const float a=fuzz9.process(x),b=fuzz6.process(x);
  CHECK(std::isfinite(a)&&std::isfinite(b));
  if(i>2000){fuzzDifference+=std::abs(a-b);fuzzEnergy+=std::abs(a);}
 }
 CHECK(fuzzEnergy>1.);
 CHECK(fuzzDifference>0.1);
 std::cout<<"PASS: realtime germanium Fuzz Face 6V/9V difference "<<fuzzDifference<<"\n";
 Circuit nmos;
 nmos.parts.push_back({"V1",PartType::DcVoltageSource,"vcc","0",9.});
 nmos.parts.push_back({"RD",PartType::Resistor,"vcc","out",2200.});
 nmos.parts.push_back({"M1",PartType::NmosTransistor,"out","in",2.0,"0"});
 RealtimeCircuitProcessor nmosProc(nmos,sr);
 float mosOff=0.f;for(int i=0;i<200;i++)mosOff=nmosProc.process(1.0f);
 float mosOn=0.f;for(int i=0;i<200;i++)mosOn=nmosProc.process(3.0f);
 CHECK(std::isfinite(mosOff)&&std::isfinite(mosOn));
 CHECK(mosOn<mosOff);
 CHECK(std::abs(mosOn-mosOff)>0.2f);
 Circuit jfetRt;
 jfetRt.parts.push_back({"V1",PartType::DcVoltageSource,"vcc","0",9.});
 jfetRt.parts.push_back({"RD",PartType::Resistor,"vcc","out",2200.});
 jfetRt.parts.push_back({"J1",PartType::NjfetTransistor,"out","in",2.0,"0"});
 RealtimeCircuitProcessor jfetProc(jfetRt,sr);
 float jfetOn=0.f;for(int i=0;i<200;i++)jfetOn=jfetProc.process(0.0f);
 float jfetPinched=0.f;for(int i=0;i<200;i++)jfetPinched=jfetProc.process(-2.5f);
 CHECK(std::isfinite(jfetOn)&&std::isfinite(jfetPinched));
 CHECK(jfetOn<jfetPinched);
 CHECK(std::abs(jfetOn-jfetPinched)>0.2f);
 Circuit op;
 op.parts.push_back({"V1",PartType::DcVoltageSource,"vcc","0",5.});
 op.parts.push_back({"V2",PartType::DcVoltageSource,"vee","0",-5.});
 op.parts.push_back({"R1",PartType::Resistor,"out","fb",10000.});
 op.parts.push_back({"R2",PartType::Resistor,"fb","0",10000.});
 op.parts.push_back({"BOP1",PartType::OpAmp,"out","in",100000.,"fb","vcc","vee"});
 RealtimeCircuitProcessor opProc(op,sr);
 float opLow=0.f;for(int i=0;i<200;i++)opLow=opProc.process(0.25f);
 float opHigh=0.f;for(int i=0;i<200;i++)opHigh=opProc.process(0.50f);
 CHECK(std::isfinite(opLow)&&std::isfinite(opHigh));
 CHECK(std::abs(opLow-0.5f)<0.08f);
 CHECK(std::abs(opHigh-1.0f)<0.12f);
 bool singular=false;
 try{
  Circuit bad;bad.parts.push_back({"R1",PartType::Resistor,"in","out",1000.});
  bad.parts.push_back({"C1",PartType::Capacitor,"floating","0",1e-7});
  RealtimeCircuitProcessor fail(bad,sr);
 }catch(const std::exception&){singular=true;}
 CHECK(singular);

 // Pin-complete PT2399: the physical resistor between VCO pin 6 and DGND
 // controls the internal delay. Source, bias and output coupling stay in MNA.
 const auto makePt2399=[&](double ohms,double volts){
  Circuit graph;
  graph.parts.push_back({"V1",PartType::DcVoltageSource,"vcc","0",volts});
  graph.parts.push_back({"R1",PartType::Resistor,"in","sig",1000.});
  graph.parts.push_back({"R2",PartType::Resistor,"sig","ref",100000.});
  graph.parts.push_back({"R3",PartType::Resistor,"vco","0",ohms});
  graph.parts.push_back({"R4",PartType::Resistor,"out","0",100000.});
  graph.parts.push_back({"C1",PartType::Capacitor,"wet","out",1e-6});
  Part ic{"X1",PartType::Pt2399,"vcc","ref",0.,"0","0","clk"};
  ic.extraPins={"vco","cc1","cc0","op1out","op1in","op2in","op2out","lpf2in","wet","lpf1out","sig"};
  graph.parts.push_back(ic);
  return graph;
 };
 auto pulsePeak=[&](double resistance,double supply){
  RealtimeCircuitProcessor processor(makePt2399(resistance,supply),sr);
  RealtimeCircuitProcessor baseline(makePt2399(resistance,supply),sr);
  double peak=0.;int first=-1;
  for(int n=0;n<40000;++n){
   // Subtract identical zero-input startup to isolate the actual impulse.
   // Coupling capacitors and the 2.5-V reference have DC transients.
   const float input=(n>=24000&&n<24100)?1.f:0.f;
   const float output=processor.process(input);
   const float quiet=baseline.process(0.f);
   CHECK(std::isfinite(output)&&std::isfinite(quiet));
   const double difference=std::abs(double(output)-double(quiet));
   if(n>=24000&&difference>0.002){peak=std::max(peak,difference);if(first<0)first=n-24000;}
  }
  return std::pair<int,double>{first,peak};
 };
 const auto fastPt=pulsePeak(2000.,5.);
 const auto slowPt=pulsePeak(10500.,5.);
 const auto offPt=pulsePeak(2000.,0.);
 std::cerr<<"PT samples fast="<<fastPt.first<<" slow="<<slowPt.first<<" off="<<offPt.first<<" peaks="<<fastPt.second<<","<<slowPt.second<<"\n";
 CHECK(fastPt.first>2400&&fastPt.first<3800);
 CHECK(slowPt.first>7000&&slowPt.first<9400);
 CHECK(slowPt.first>fastPt.first+3000);
 CHECK(fastPt.second>0.05&&slowPt.second>0.05);
 CHECK(offPt.first==-1);
 std::cout<<"PASS PT2399 physical MNA supply, 16-pin netlist and pin6 controlled audible impulse time\n";

 const auto makeBbd=[&](double power,double oscillator){
  Circuit c;
  c.parts.push_back({"V1",PartType::DcVoltageSource,"vneg","0",power});
  c.parts.push_back({"R1",PartType::Resistor,"vneg","bias",100000.});
  c.parts.push_back({"R2",PartType::Resistor,"bias","0",100000.});
  c.parts.push_back({"C1",PartType::Capacitor,"in","bias",1e-6});
  c.parts.push_back({"R3",PartType::Resistor,"osc","0",oscillator});
  c.parts.push_back({"C2",PartType::Capacitor,"oscCap","0",100e-12});
  c.parts.push_back({"C3",PartType::Capacitor,"bbdOut","out",1e-6});
  c.parts.push_back({"R5",PartType::Resistor,"out","0",100000.});
  c.parts.push_back({"R6",PartType::Resistor,"oscIn","0",22000.});
  c.parts.push_back({"C4",PartType::Capacitor,"bbdOut2","out",1e-6});
  Part clk{"XCLK1",PartType::Mn3101,"0","cp1",0.,"vneg","cp2","oscCap"};
  clk.extraPins={"osc","oscIn","vgg"};
  c.parts.push_back(clk);
  Part bb{"XBB1",PartType::Mn3007,"0","cp1",0.,"bias","vgg","vneg"};
  bb.extraPins={"cp2","bbdOut","bbdOut2"};
  c.parts.push_back(bb);
  return c;
 };
 RealtimeCircuitProcessor chorusBbd(makeBbd(-15.,22000.),48000.);
 RealtimeCircuitProcessor chorusSlow(makeBbd(-15.,39000.),48000.);
 RealtimeCircuitProcessor chorusBaseline(makeBbd(-15.,22000.),48000.);
 RealtimeCircuitProcessor chorusSlowBaseline(makeBbd(-15.,39000.),48000.);
 RealtimeCircuitProcessor chorusNoPower(makeBbd(0.,22000.),48000.);
 RealtimeCircuitProcessor chorusNoPowerBaseline(makeBbd(0.,22000.),48000.);
 double bbdPeak=0.,slowPeak=0.,offPeak=0.;int firstBbd=-1,firstSlow=-1;
 for(int i=0;i<12500;++i){
  const float x=(i>=3500&&i<3650)?0.4f:0.f;
  const float wet=chorusBbd.process(x)-chorusBaseline.process(0.f);
  const float slow=chorusSlow.process(x)-chorusSlowBaseline.process(0.f);
  const float off=chorusNoPower.process(x)-chorusNoPowerBaseline.process(0.f);
  CHECK(std::isfinite(wet)&&std::isfinite(slow)&&std::isfinite(off));
  if(i>3500){
   const double m=std::abs(double(wet)),q=std::abs(double(slow));
   bbdPeak=std::max(bbdPeak,m);slowPeak=std::max(slowPeak,q);
   offPeak=std::max(offPeak,std::abs(double(off)));
   if(firstBbd<0&&m>0.001)firstBbd=i-3500;
   if(firstSlow<0&&q>0.001)firstSlow=i-3500;
  }
 }
 std::cerr<<"BBD measured first="<<firstBbd<<", slow="<<firstSlow<<", peaks="<<bbdPeak<<","<<slowPeak<<", off="<<offPeak<<"\\n";
 CHECK(offPeak<0.02);
 CHECK(bbdPeak>0.001&&slowPeak>0.001);
 CHECK(firstBbd>250&&firstSlow>firstBbd+150);

 // A floating BBD supply domain must behave identically when its local
 // GND, VDD, oscillator ground and bias are all translated by +5 V.
 // The audio input/output stay AC-coupled to the global 0 V domain.
 const auto translateBbd=[&](double offset){
  Circuit c=makeBbd(-15.+offset,22000.);
  c.parts.push_back({"V2",PartType::DcVoltageSource,"bgnd","0",offset});
  for(auto& part:c.parts){
   if(part.type==PartType::DcVoltageSource||part.reference=="R5")continue;
   auto move=[&](std::string& node){if(node=="0")node="bgnd";};
   move(part.pin1);move(part.pin2);
   if(part.type==PartType::Mn3007||part.type==PartType::Mn3101){
    move(part.pin3);move(part.pin4);move(part.pin5);
    for(auto& node:part.extraPins)move(node);
   }
  }
  return c;
 };
 RealtimeCircuitProcessor shiftedBbd(translateBbd(5.),sr);
 RealtimeCircuitProcessor shiftedBaseline(translateBbd(5.),sr);
 RealtimeCircuitProcessor unshiftedBbd(makeBbd(-15.,22000.),sr);
 RealtimeCircuitProcessor unshiftedBaseline(makeBbd(-15.,22000.),sr);
 double translatedPeak=0.,referencePeak=0.,maxTranslatedError=0.;
 for(int i=0;i<14500;++i){
  const float input=(i>=3600&&i<3900)?0.3f:0.f;
  const double translated=double(shiftedBbd.process(input))-shiftedBaseline.process(0.f);
  const double reference=double(unshiftedBbd.process(input))-unshiftedBaseline.process(0.f);
  CHECK(std::isfinite(translated)&&std::isfinite(reference));
  if(i>=4100){
   translatedPeak=std::max(translatedPeak,std::abs(translated));
   referencePeak=std::max(referencePeak,std::abs(reference));
   maxTranslatedError=std::max(maxTranslatedError,std::abs(translated-reference));
  }
 }
 CHECK(referencePeak>0.001);
 CHECK(translatedPeak>0.001);
 CHECK(maxTranslatedError<0.02);
 std::cout<<"PASS MN3007 AC signal relative to pin-1 GND under +5 V common-mode translation\\n";
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
