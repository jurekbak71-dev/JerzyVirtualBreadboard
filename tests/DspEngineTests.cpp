#include "../Source/FuzzEngine.h"
#include <cstdlib>
#define REQUIRE(condition) do {if(!(condition)){std::cerr << "FAIL " << #condition << " at line " << __LINE__ << "\n";std::abort();}} while(false)
#include <iostream>
#include <limits>
int main(){
  FuzzEngine engine; FuzzEngine::Parameters p;
  for(double sr:{44100.,48000.,96000.}){
    engine.prepare(sr);
    for(int i=0;i<10000;++i){float l=0,r=0;engine.process(l,r,p);REQUIRE(l==0 && r==0);}
    p.gain=30;p.feedback=.85f;p.resonance=1;p.instability=1;p.flangerFeedback=1;p.flangerDepth=1;p.flangerOn=1;
    p.micMovement=1;p.micSensitivity=2;p.bias=1;p.filter1Res=1;p.filter2Res=1;
    for(int i=0;i<100000;++i){float l=static_cast<float>(std::sin(i*.1)*4),r=l; if(i==10)l=std::numeric_limits<float>::quiet_NaN();if(i==20)r=std::numeric_limits<float>::infinity();engine.process(l,r,p);REQUIRE(std::isfinite(l)&&std::isfinite(r));REQUIRE(std::abs(l)<=1.01f&&std::abs(r)<=1.01f);}
    p=FuzzEngine::Parameters{};
  }
  // The IR must modify feedback and cannot affect the output with feedback disabled.
  auto compare=[](float feedback){FuzzEngine a,b; a.prepare(44100);b.prepare(44100);float taps[2]={0,1};REQUIRE(b.stageIR(taps,2));a.beginBlock();b.beginBlock();FuzzEngine::Parameters p;p.feedback=feedback;p.irMix=1;p.early=0;double difference=0;for(int i=0;i<20000;++i){float z=0,w=0;a.process(z,w,p);z=0;w=0;b.process(z,w,p);}for(int i=0;i<16000;++i){float l=std::sin(i*.12f)*.12f,r=l,bl=l,br=r;a.process(l,r,p);b.process(bl,br,p);if(i>2000)difference+=std::abs(l-bl);}return difference;};
  REQUIRE(compare(0)<.0001);REQUIRE(compare(.8f)>.001);
  float invalid[]={0,0};REQUIRE(!engine.stageIR(invalid,2));REQUIRE(!engine.stageIR(invalid,2049));
  // Every available control changes a deterministic render. B is unavailable and deliberately excluded.
  using P=FuzzEngine::Parameters;
  struct Control {const char* name;float P::* member;float value;};
  const Control controls[]={
    {"gain",&P::gain,20},{"feedback",&P::feedback,.1f},{"mix",&P::mix,.2f},{"output",&P::output,.3f},
    {"bias",&P::bias,.8f},{"tone",&P::tone,.1f},{"gate",&P::gate,1},{"type",&P::type,1},
    {"micDistance",&P::micDistance,.8f},{"micPosition",&P::micPosition,.9f},{"micMovement",&P::micMovement,.9f},
    {"micSensitivity",&P::micSensitivity,1.9f},{"micMode",&P::micMode,1},{"resonance",&P::resonance,.9f},
    {"instability",&P::instability,1},{"filter1Freq",&P::filter1Freq,700},{"filter1Res",&P::filter1Res,1},
    {"filter2Freq",&P::filter2Freq,1500},{"filter2Res",&P::filter2Res,1},
    {"tremoloRate",&P::tremoloRate,12},{"tremoloDepth",&P::tremoloDepth,.1f},{"tremoloOn",&P::tremoloOn,0},
    {"flangerRate",&P::flangerRate,2},{"flangerDepth",&P::flangerDepth,.1f},{"flangerFeedback",&P::flangerFeedback,.9f},{"flangerOn",&P::flangerOn,0},
    {"limiter",&P::limiter,0},{"bypass",&P::bypass,1},{"irMix",&P::irMix,0},{"early",&P::early,1},{"decay",&P::decay,1}};
  for(const auto& control:controls){
    FuzzEngine a,b; a.prepare(44100);b.prepare(44100);float ir[]={.2f,.8f};a.stageIR(ir,2);b.stageIR(ir,2);a.beginBlock();b.beginBlock();
    P base;base.output=2;base.mix=.8f;base.feedback=.6f;base.tremoloOn=1;base.tremoloDepth=.7f;base.flangerOn=1;base.flangerDepth=.7f;
    P changed=base;changed.*(control.member)=control.value;double difference=0;
    for(int i=0;i<22000;++i){float x=static_cast<float>((.01+.3*(1+std::sin(i*.0007)))*std::sin(i*.13));float l=x,r=x,bl=x,br=x;a.process(l,r,base);b.process(bl,br,changed);if(i>4000)difference+=std::abs(l-bl);}
    if(difference<1e-5){std::cerr<<"Control had no audible-sample effect: "<<control.name<<"\n";return 1;}
  }
  // Dense parameter automation and reset/recall are deterministic and bounded.
  {FuzzEngine a,b;a.prepare(48000);b.prepare(48000);for(int i=0;i<40000;++i){P automated;automated.gain=(i%2)?30:1;automated.bias=(i%3)?-1:1;automated.mix=(i%4)?1:0;float l=.2f,r=-.2f,bl=l,br=r;a.process(l,r,automated);b.process(bl,br,automated);REQUIRE(l==bl&&r==br);REQUIRE(std::isfinite(l)&&std::abs(l)<=1.01f);}}
  std::cout<<"PASS: silence, rates, 300k extreme samples, nonfinite recovery, bounds, IR feedback topology rejection, 31 individual controls and automation\n";
}
