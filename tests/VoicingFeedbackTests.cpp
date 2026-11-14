#include "../Source/FuzzEngine.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <algorithm>

#define EXPECT(c) do {if(!(c)){std::fprintf(stderr,"FAIL: %s at %d\n",#c,__LINE__);return 1;}}while(false)
static std::array<float,16384> render(float voice,float bias,float feedback){
    auto* e=new FuzzEngine; e->prepare(48000);
    FuzzEngine::Parameters p;
    p.type=voice;p.bias=bias;p.gain=11;p.feedback=feedback;p.mix=1;p.output=.7f;
    p.early=1;p.irMix=.5f;p.micDistance=.1f;p.micSensitivity=1.5f;
    std::array<float,16384> out{};
    for(int i=0;i<16384;++i){
        float x=.13f*(std::sin(i*.091f)+.4f*std::sin(i*.017f));
        float l=x,r=x;e->process(l,r,p);out[static_cast<size_t>(i)]=l;
    }
    delete e;return out;
}
static double difference(const std::array<float,16384>& a,const std::array<float,16384>& b){
    double s=0;for(int i=2048;i<16384;++i)s+=std::abs(a[static_cast<size_t>(i)]-b[static_cast<size_t>(i)]);
    return s;
}
int main(){
    // Exact selector numbers 0..3 must not collapse to only two curves.
    std::array<std::array<float,16384>,4> voices{};
    for(int k=0;k<4;++k)voices[static_cast<size_t>(k)]=render(float(k),0,0);
    for(int k=0;k<4;++k)for(int j=k+1;j<4;++j){const double d=difference(voices[k],voices[j]);EXPECT(d>1);}
    for(int k=0;k<4;++k){
        const auto minus=render(float(k),-1,0);const auto plus=render(float(k),1,0);
        EXPECT(difference(minus,plus)>1);
        for(float v:minus)EXPECT(std::isfinite(v)&&std::abs(v)<=.9951f);
        for(float v:plus)EXPECT(std::isfinite(v)&&std::abs(v)<=.9951f);
    }
    // Active feedback should influence signal while output stays bounded.
    EXPECT(difference(render(2,0,.85f),render(2,0,0))>.25);
    {FuzzEngine e;e.prepare(48000);FuzzEngine::Parameters p;
      p.feedback=.85f;p.gain=30;p.micSensitivity=2;p.micDistance=0;p.instability=1;
      for(int i=0;i<200000;++i){float l=i%37==0?8.f:.2f*std::sin(i*.1f),r=l;
        e.process(l,r,p);EXPECT(std::isfinite(l)&&std::abs(l)<=.9951f);
        EXPECT(std::isfinite(e.getFeedbackActivity()));EXPECT(e.getFeedbackActivity()>=0&&e.getFeedbackActivity()<=1);}}
    // Bypass is no loophole for a 0dBFS protection ceiling.
    {FuzzEngine e;e.prepare(48000);FuzzEngine::Parameters p;p.bypass=1;p.output=2;p.limiter=0;
      for(int i=0;i<5000;++i){float l=7,r=-7;e.process(l,r,p);EXPECT(std::abs(l)<=.9951f&&std::abs(r)<=.9951f);}}
    // Direct finite but extreme host values must not poison running DSP state.
    {FuzzEngine e;e.prepare(48000);FuzzEngine::Parameters p;p.gain=std::numeric_limits<float>::max();
      p.type=std::numeric_limits<float>::max();p.feedback=std::numeric_limits<float>::max();
      p.filter1Freq=std::numeric_limits<float>::max();p.micMovement=std::numeric_limits<float>::max();
      for(int i=0;i<2000;++i){float l=.1f,r=.1f;e.process(l,r,p);EXPECT(std::isfinite(l)&&std::isfinite(r));}}
    std::puts("PASS: 4 independent voicings, BIAS polarity and level effects, feedback tracking/bounds, limiter-off/bypass safety, extreme finite host values");
}
