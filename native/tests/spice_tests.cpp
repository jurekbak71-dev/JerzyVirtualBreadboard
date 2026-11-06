#include "spice.hpp"
#include <stdexcept>
#define CHECK(expr) do { if(!(expr)) throw std::runtime_error("Check failed: " #expr); } while(false)
#include <cmath>
#include <string>
int main(){
 using namespace vbal;
 Audio audio{10000,1,{0.f,0.5f,-0.5f,0.f}};
 auto netlist=makeRcSpiceNetlist(audio,10000.,1e-8,0);
 CHECK(netlist.find("Vin in 0 PWL(0 0")!=std::string::npos);
 CHECK(netlist.find("0.0001 0.5")!=std::string::npos);
 CHECK(netlist.find("R1 in out 10000")!=std::string::npos);
 CHECK(netlist.find("C1 out 0 1e-08")!=std::string::npos);
 CHECK(netlist.find("tran 0.0001 0.0004")!=std::string::npos);
 CHECK(netlist.find("wrdata rc_output.dat v(out)")!=std::string::npos);
 Audio stereo{48000,2,{0.f,1.f,0.5f,0.75f}};
 auto right=makeRcSpiceNetlist(stereo,1000.,1e-8,1);
 CHECK(right.find("0 1")!=std::string::npos);
 CHECK(right.find("2.0833333333333333e-05 0.75")!=std::string::npos);
 auto bad=[&](const Audio& a,double r,double c,size_t channel){try{makeRcSpiceNetlist(a,r,c,channel);return false;}catch(const std::invalid_argument&){return true;}};
 CHECK(bad(audio,-1,1e-8,0)); CHECK(bad(audio,1,0,0)); CHECK(bad(audio,1,1e-8,1));
 CHECK(bad(Audio{10000,1,{}},1,1e-8,0));
 CHECK(bad(Audio{10000,1,{NAN}},1,1e-8,0));
 CHECK(bad(Audio{10000,1,std::vector<float>(4097,0.f)},1,1e-8,0));
}
