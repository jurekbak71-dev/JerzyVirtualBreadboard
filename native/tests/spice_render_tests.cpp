#include "spice_render.hpp"
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#define CHECK(e) do { if(!(e)) throw std::runtime_error("Check failed: " #e); } while(false)
int main(){
 using namespace vbal;
 auto samples=parseSpiceWrdata(" 0 0\n 0.000025 0.25\n 0.0001 1\n 0.0002 0\n");
 CHECK(samples.size()==4);
 auto a=resampleSpiceOutput(samples,10000,3);
 CHECK(a.size()==3);CHECK(std::abs(a[0])<1e-8);CHECK(std::abs(a[1]-1)<1e-6);CHECK(std::abs(a[2])<1e-6);
 auto repeated=parseSpiceWrdata("0 0\n0 0.25\n0.0001 0.5\n");
 CHECK(std::abs(resampleSpiceOutput(repeated,10000,2)[0]-.25)<1e-8);
 bool threw=false;try{parseSpiceWrdata("0 0\n0.001 nan\n");}catch(const std::runtime_error&){threw=true;}CHECK(threw);
 threw=false;try{parseSpiceWrdata("0.1 0\n0.05 1\n");}catch(const std::runtime_error&){threw=true;}CHECK(threw);
 threw=false;try{resampleSpiceOutput(samples,48000,100);}catch(const std::runtime_error&){threw=true;}CHECK(threw);
}
