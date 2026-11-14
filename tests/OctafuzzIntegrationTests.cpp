#include "../Source/FuzzEngine.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <limits>

#define REQUIRE(c) do {if(!(c)){std::fprintf(stderr,"FAIL %s line %d\n",#c,__LINE__);return 1;}}while(false)
static constexpr int rate=48000;
static constexpr float pi=3.14159265358979323846f;
using Audio=std::array<float,rate>;

static std::unique_ptr<Audio> render(bool enabled,int mode,float level=0.8f) {
    auto engine=std::make_unique<FuzzEngine>();
    engine->prepare(rate);
    FuzzEngine::Parameters p;
    p.gain=7.f;p.type=1.f;p.bias=0.f;p.feedback=0.f;
    p.early=0.f;p.irMix=0.f;p.tone=.85f;p.filter1Freq=9000.f;
    p.filter2Freq=20.f;p.mix=1.f;p.output=.55f;
    p.octOn=enabled?1.f:0.f;p.octMode=float(mode);p.octLevel=level;
    auto output=std::make_unique<Audio>();
    for(int i=0;i<rate;++i){
        const float x=.13f*std::sin(2*pi*220.f*i/rate);
        float left=x,right=x;
        engine->process(left,right,p);
        (*output)[static_cast<size_t>(i)]=left;
        if(!std::isfinite(left)||std::abs(left)>.9951f)std::abort();
    }
    return output;
}
static double component(const Audio& x,double hz){
    double r=0,im=0;
    for(int i=rate/2;i<rate;++i){
        const double a=2*pi*hz*(i-rate/2)/rate;
        r+=x[static_cast<size_t>(i)]*std::cos(a);
        im+=x[static_cast<size_t>(i)]*std::sin(a);
    }
    return std::sqrt(r*r+im*im)/(rate/4.);
}
static double deviation(const Audio& a,const Audio& b){
    double d=0;for(int i=rate/2;i<rate;++i)d+=std::abs(a[static_cast<size_t>(i)]-b[static_cast<size_t>(i)]);
    return d/(rate/2.);
}
int main(){
    const auto off=render(false,0), offBoth=render(false,2),zeroLevel=render(true,1,0);
    // Mode/level cannot influence fuzz when the octaver is switched off.
    REQUIRE(*off==*offBoth);
    // LEVEL uses a short anti-click smoothing transition. Verify that zero
    // yields no steady-state contribution rather than requiring an abrupt cut.
    REQUIRE(deviation(*off,*zeroLevel)<.000001);
    const auto down=render(true,0),up=render(true,1),both=render(true,2);
    const auto base110=component(*off,110),base440=component(*off,440);
    const auto down110=component(*down,110),up440=component(*up,440);
    const auto both110=component(*both,110),both440=component(*both,440);
    std::printf("base110 %.5f base440 %.5f | DOWN110 %.5f UP440 %.5f BOTH110 %.5f BOTH440 %.5f\n",
                base110,base440,down110,up440,both110,both440);
    REQUIRE(down110>base110+.002);
    REQUIRE(up440>base440+.002);
    REQUIRE(both110>base110+.001);
    REQUIRE(both440>base440+.001);
    REQUIRE(deviation(*down,*up)>.001);
    // Dense host automation, hostile values, both channels and bypass safety.
    auto engine=std::make_unique<FuzzEngine>();engine->prepare(96000);
    FuzzEngine::Parameters p;
    for(int i=0;i<100000;++i){
        p.octOn=(i%15)<10?1.f:0.f;p.octMode=float(i%3);p.octLevel=float(i%11)/10.f;
        p.bias=(i%19)/9.f-1.f;p.gate=float(i%7)/6.f;p.gain=1.f+float(i%30);
        if(i%23==0)p.octLevel=std::numeric_limits<float>::infinity();
        float left=(i%97==0)?5.f:.17f*std::sin(float(i)*.12f);
        float right=-left;engine->process(left,right,p);
        REQUIRE(std::isfinite(left)&&std::isfinite(right));
        REQUIRE(std::abs(left)<=.9951f&&std::abs(right)<=.9951f);
    }
    std::puts("PASS: OCT DOWN/UP/BOTH pre/post fuzz, strict OFF isolation, level zero, automation and bounded audio");
}
