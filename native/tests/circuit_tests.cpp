#include "circuit.hpp"
#include <stdexcept>
#include <string>
#define CHECK(x) do { if(!(x)) throw std::runtime_error("failed: " #x); } while(false)
int main() {
 using namespace vbal;
 Circuit rc=makeRcCircuit(10000,1e-8,false);
 validateCircuit(rc);
 auto net=generateSpiceElements(rc);
 CHECK(net.find("R1 in out 10000")!=std::string::npos);
 CHECK(net.find("C1 out 0 1e-08")!=std::string::npos);
 CHECK(net.find("D1")==std::string::npos);
 Circuit clip=makeRcCircuit(10000,1e-8,true);
 auto diodes=generateSpiceElements(clip);
 CHECK(diodes.find("D1 out 0 DGEN")!=std::string::npos);
 CHECK(diodes.find("D2 0 out DGEN")!=std::string::npos);
 CHECK(diodes.find(".model DGEN D(")!=std::string::npos);
 auto rejected=[](Circuit c){try{validateCircuit(c);return false;}catch(const std::invalid_argument&){return true;}};
 auto duplicate=rc;duplicate.parts[1].reference="R1";CHECK(rejected(duplicate));
 auto shorted=rc;shorted.parts[0].pin2="in";CHECK(rejected(shorted));
 auto badValue=rc;badValue.parts[0].value=-10;CHECK(rejected(badValue));
 auto badNode=rc;badNode.parts[0].pin1="in;quit";CHECK(rejected(badNode));
 auto floating=rc;floating.parts[1].pin2="floating";CHECK(rejected(floating));
 auto missingOutput=rc;missingOutput.outputNode="missing";CHECK(rejected(missingOutput));
}
