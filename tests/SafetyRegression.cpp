#include "../Source/FuzzEngine.h"
#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    FuzzEngine engine;
    engine.prepare(48000);
    FuzzEngine::Parameters p;
    p.feedback=0; p.mix=1; p.gain=9;
    for (int i=0;i<2000;++i) { float l=.15f*std::sin(float(i)*.07f), r=l; engine.process(l,r,p); }
    // An invalid host value must not permanently poison smoothed DSP state.
    p.gain=std::numeric_limits<float>::quiet_NaN();
    {float l=.15f,r=l;engine.process(l,r,p);}
    p.gain=9;
    double sum=0;
    for(int i=0;i<30000;++i) {
        float l=.15f*std::sin(float(i)*.071f),r=l;
        engine.process(l,r,p);
        if(!std::isfinite(l)||!std::isfinite(r)){std::fprintf(stderr,"FAIL invalid output at %d\n",i);return 1;}
        if(i>=10000)sum+=std::abs(l);
    }
    if(sum<100) {std::fprintf(stderr,"FAIL state poisoned, recovered energy %.9f\n",sum);return 2;}
    if(engine.stageIR(nullptr, 1)) {std::fprintf(stderr,"FAIL accepted a null IR pointer\n");return 3;}
    engine.prepare(48000);
    p=FuzzEngine::Parameters{};
    std::array<float,2> invalidRoom{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()};
    for(int i=0;i<2500;++i) {
        float l=.12f*std::sin(float(i)*.11f),r=l;
        engine.process(l,r,p,true,&invalidRoom);
        if(!std::isfinite(l)||!std::isfinite(r)){std::fprintf(stderr,"FAIL nonfinite external IR audio at %d\n",i);return 4;}
    }
    invalidRoom={0.f,0.f};
    double resumed=0;
    for(int i=0;i<20000;++i) {
        float l=.12f*std::sin(float(i)*.11f),r=l;
        engine.process(l,r,p,true,&invalidRoom);
        if(i>=5000) resumed+=std::abs(l);
    }
    if(resumed<50){std::fprintf(stderr,"FAIL external IR poisoned persistent state, energy %.9f\n",resumed);return 5;}
    std::puts("PASS invalid parameter recovery, external room isolation, and null IR rejection");
}
