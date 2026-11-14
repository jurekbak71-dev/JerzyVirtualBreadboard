#include "../Source/AnalogOctave.h"
#include <array>
#include <cmath>
#include <cstdio>

#define REQUIRE(c) do { if (!(c)) { std::fprintf(stderr,"FAIL %s line %d\n", #c, __LINE__); return 1; } } while(0)
static constexpr float pi = 3.14159265358979323846f;
static double component(const std::array<float,48000>& x, float frequency) {
    double re=0, im=0;
    for(int i=24000;i<48000;++i){
        const double p=2.0*pi*frequency*(i-24000)/48000.0;
        re+=x[static_cast<size_t>(i)]*std::cos(p);
        im+=x[static_cast<size_t>(i)]*std::sin(p);
    }
    return std::sqrt(re*re+im*im)/12000.0;
}
int main(){
    std::array<std::array<float,48000>,3> outputs{};
    for(int mode=0;mode<3;++mode){
        AnalogOctave octave;
        for(int i=0;i<48000;++i){
            const float x=.25f*std::sin(2*pi*220*i/48000.f);
            outputs[static_cast<size_t>(mode)][static_cast<size_t>(i)]=octave.process(x,48000.f,mode);
            REQUIRE(std::isfinite(outputs[static_cast<size_t>(mode)][static_cast<size_t>(i)]));
        }
    }
    const double down110=component(outputs[0],110), down440=component(outputs[0],440);
    const double up110=component(outputs[1],110), up440=component(outputs[1],440);
    const double both110=component(outputs[2],110), both440=component(outputs[2],440);
    std::printf("DOWN: 110=%.5f 440=%.5f UP: 110=%.5f 440=%.5f BOTH 110=%.5f 440=%.5f\n",down110,down440,up110,up440,both110,both440);
    REQUIRE(down110>.02); REQUIRE(down110 > down440*4);
    REQUIRE(up440>.02); REQUIRE(up440>up110*5);
    REQUIRE(both110>.01); REQUIRE(both440>.01);
    for(double rate: {44100.,96000.,192000.}) {
        AnalogOctave o;for(int i=0;i<100000;++i) {
            float value=.7f*std::sin(2*pi*110.f*i/float(rate));
            value=o.process(value,float(rate),i%3);
            REQUIRE(std::isfinite(value));REQUIRE(std::abs(value)<=1.50001f);
        }
    }
    AnalogOctave o;for(int i=0;i<48000;++i) REQUIRE(o.process(0.f,48000.f,2)==0);
    std::puts("PASS analog octave down/up/both spectral components, rates, silence and bounds");
}
