#include "../Source/AnalogFuzzStage.h"
#include <array>
#include <cmath>
#include <cstdio>
#define REQUIRE(c) do {if(!(c)) {std::fprintf(stderr,"FAIL: %s line %d\n",#c,__LINE__);return 1;}} while(0)
static constexpr float pi=3.14159265358979323846f;
static double render(float v,float bias,float gate,float amplitude){
 AnalogFuzzStage f;double energy=0;float envelope=0;
 for(int i=0;i<48000;++i){float signal=amplitude*std::sin(2*pi*220*i/48000.0f);
    envelope+=(1.f-std::exp(-2*pi*15/48000.0f))*(std::abs(signal)-envelope);
    const float result=f.process(signal*12,bias,gate,v,envelope,48000.f);
    if(i>10000) energy+=std::abs(result);}
 return energy/38000.;}
int main(){
 std::array<double,4> voices{};
 for(int k=0;k<4;++k)voices[static_cast<size_t>(k)]=render(float(k),0,0,.13f);
 for(int i=0;i<4;++i)for(int j=i+1;j<4;++j)REQUIRE(std::abs(voices[i]-voices[j])>.005);
 const auto minus=render(1,-.95f,0,.13f),plus=render(1,.95f,0,.13f);
 std::printf("VOICES: %.4f %.4f %.4f %.4f, biased %.4f %.4f\n",voices[0],voices[1],voices[2],voices[3],minus,plus);
 const auto open=render(3,.3f,0,.012f),gated=render(3,.3f,1,.012f);
 const auto loud=render(3,.3f,1,.4f);
 std::printf("GATE: low ungated %.5f gated %.5f, loud gated %.5f\n",open,gated,loud);
 REQUIRE(gated<open*.3); REQUIRE(loud>gated*10);
 const auto low=render(1,0,0,.1f),high=render(1,0,0,.4f);
 REQUIRE(high<low*3.5);
 AnalogFuzzStage f;for(int i=0;i<10000;++i)REQUIRE(f.process(0.f,0.f,0.f,0.f,0.f,48000.f)==0.f);
 std::puts("PASS: distinct voices, dynamic sag compression, gated starvation, silence");
}
