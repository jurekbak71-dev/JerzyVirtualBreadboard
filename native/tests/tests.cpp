#include "audio.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
int main(){
 using namespace vbal;
 Audio a{48000,2,{1,0,1,0,1,0}};rcLowpass(a,10000,1e-8); assert(a.samples[0]>0 && a.samples[0]<1);assert(a.samples[1]==0);assert(a.samples[2]>a.samples[0]);assert(a.samples[3]==0);
 auto f=std::filesystem::temp_directory_path()/"vbal_test_roundtrip.wav";writePcm16Wav(f,a);auto b=readPcmWav(f);assert(b.channels==2 && b.sampleRate==48000 && b.samples.size()==a.samples.size());for(size_t i=0;i<a.samples.size();++i)assert(std::abs(b.samples[i]-a.samples[i])<0.00004f);std::filesystem::remove(f);
 auto invalid=[](auto fun){try{fun();return false;}catch(const std::runtime_error&){return true;}};
 assert(invalid([&]{auto z=a;rcLowpass(z,0,1e-8);}));assert(invalid([&]{auto z=a;rcLowpass(z,1,-1);}));assert(invalid([&]{auto z=a;rcLowpass(z,std::numeric_limits<double>::quiet_NaN(),1e-8);}));
 assert(invalid([&]{Audio z{48000,2,{0,0,1}};rcLowpass(z,10,1e-8);}));assert(invalid([&]{Audio z{48000,1,{std::numeric_limits<float>::infinity()}};rcLowpass(z,10,1e-8);}));
 assert(invalid([&]{readPcmWav("does_not_exist.wav");}));
 Audio c{48000,1,{-2,0,2}};writePcm16Wav(f,c);auto d=readPcmWav(f);assert(d.samples[0]<=-0.999 && d.samples[2]>=0.999);std::filesystem::remove(f);
 std::cout<<"Native audio tests PASS\n";
}
