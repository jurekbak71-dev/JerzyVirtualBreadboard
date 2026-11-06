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
 auto exported=exportCircuitSpice(clip);
 CHECK(exported.find("Vin in 0 SIN(0 1 1000) AC 1")!=std::string::npos);
 CHECK(exported.find(".model DGEN")!=std::string::npos);
 auto bom=exportCircuitBomCsv(clip);
 CHECK(bom.find("Reference,Type,Value,Unit,Pin1,Pin2")!=std::string::npos);
 CHECK(bom.find("R1,Resistor,10000,ohm,in,out")!=std::string::npos);
 CHECK(bom.find("D2,SiliconDiode,,model:DGEN,0,out")!=std::string::npos);
 auto wiring=exportCircuitWiring(clip);
 CHECK(wiring.find("INPUT.signal")!=std::string::npos);
 CHECK(wiring.find("D1.1")!=std::string::npos);
 CHECK(wiring.find("OUTPUT.ground")!=std::string::npos);
 auto rejected=[](Circuit c){try{validateCircuit(c);return false;}catch(const std::invalid_argument&){return true;}};
 auto duplicate=rc;duplicate.parts[1].reference="R1";CHECK(rejected(duplicate));
 auto shorted=rc;shorted.parts[0].pin2="in";CHECK(rejected(shorted));
 auto badValue=rc;badValue.parts[0].value=-10;CHECK(rejected(badValue));
 auto badNode=rc;badNode.parts[0].pin1="in;quit";CHECK(rejected(badNode));
 auto floating=rc;floating.parts[1].pin2="floating";CHECK(rejected(floating));
 auto missingOutput=rc;missingOutput.outputNode="missing";CHECK(rejected(missingOutput));
}
