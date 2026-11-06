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
 auto parsed=parseEditorSpiceElements(generateSpiceElements(clip));
 CHECK(parsed.parts.size()==4);
 CHECK(generateSpiceElements(parsed)==generateSpiceElements(clip));
 bool invalidDeck=false;
 try{parseEditorSpiceElements("R1 in out 10000\\nC1 out 0 1e-8\\n.control\\nquit\\n");}
 catch(const std::invalid_argument&){invalidDeck=true;}
 CHECK(invalidDeck);
 invalidDeck=false;
 try{parseEditorSpiceElements("R1 in out 10000\\nC1 out 0 1e-8\\n.save all\\n");}
 catch(const std::invalid_argument&){invalidDeck=true;}
 CHECK(invalidDeck);
 invalidDeck=false;
 try{parseEditorSpiceElements("R1 in out NaN\\nC1 out 0 1e-8\\n");}
 catch(const std::invalid_argument&){invalidDeck=true;}
 CHECK(invalidDeck);
 Circuit extended;
 extended.parts.push_back({"L1",PartType::Inductor,"in","out",0.01});
 extended.parts.push_back({"DLED1",PartType::Led,"out","0",0});
 validateCircuit(extended);
 auto extendedNet=generateSpiceElements(extended);
 CHECK(extendedNet.find("L1 in out 0.01")!=std::string::npos);
 CHECK(extendedNet.find("DLED1 out 0 LEDGEN")!=std::string::npos);
 CHECK(extendedNet.find(".model LEDGEN")!=std::string::npos);
 auto extendedParsed=parseEditorSpiceElements(extendedNet);
 CHECK(extendedParsed.parts.size()==2);
 CHECK(extendedParsed.parts[0].type==PartType::Inductor);
 CHECK(extendedParsed.parts[1].type==PartType::Led);
 auto extendedBom=exportCircuitBomCsv(extended);
 CHECK(extendedBom.find("L1,Inductor,0.01,H,in,out")!=std::string::npos);
 CHECK(extendedBom.find("DLED1,LED,,model:LEDGEN,out,0")!=std::string::npos);
 Circuit diodeSet;
 diodeSet.parts.push_back({"R1",PartType::Resistor,"in","out",1000.});
 diodeSet.parts.push_back({"DSCH1",PartType::SchottkyDiode,"out","n1",0.});
 diodeSet.parts.push_back({"DGE1",PartType::GermaniumDiode,"n1","n2",0.});
 diodeSet.parts.push_back({"DZ1",PartType::ZenerDiode,"n2","0",0.});
 validateCircuit(diodeSet);
 const auto diodeSetNet=generateSpiceElements(diodeSet);
 CHECK(diodeSetNet.find("DSCH1 out n1 SCHGEN")!=std::string::npos);
 CHECK(diodeSetNet.find("DGE1 n1 n2 GEGEN")!=std::string::npos);
 CHECK(diodeSetNet.find("DZ1 n2 0 Z5V1")!=std::string::npos);
 CHECK(diodeSetNet.find(".model SCHGEN")!=std::string::npos);
 CHECK(diodeSetNet.find(".model GEGEN")!=std::string::npos);
 CHECK(diodeSetNet.find(".model Z5V1")!=std::string::npos);
 const auto diodeSetParsed=parseEditorSpiceElements(diodeSetNet);
 CHECK(diodeSetParsed.parts[1].type==PartType::SchottkyDiode);
 CHECK(diodeSetParsed.parts[2].type==PartType::GermaniumDiode);
 CHECK(diodeSetParsed.parts[3].type==PartType::ZenerDiode);
 auto rejected=[](Circuit c){try{validateCircuit(c);return false;}catch(const std::invalid_argument&){return true;}};
 auto duplicate=rc;duplicate.parts[1].reference="R1";CHECK(rejected(duplicate));
 auto shorted=rc;shorted.parts[0].pin2="in";CHECK(rejected(shorted));
 auto badValue=rc;badValue.parts[0].value=-10;CHECK(rejected(badValue));
 auto badNode=rc;badNode.parts[0].pin1="in;quit";CHECK(rejected(badNode));
 auto floating=rc;floating.parts[1].pin2="floating";CHECK(rejected(floating));
 auto missingOutput=rc;missingOutput.outputNode="missing";CHECK(rejected(missingOutput));
}
