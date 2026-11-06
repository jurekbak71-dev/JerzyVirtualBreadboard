#include "circuit.hpp"
#include <cmath>
#include <iomanip>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace vbal {
namespace {
bool validToken(const std::string& s) {
 if(s.empty()||s.size()>63)return false;
 for(char ch:s) if(!(ch>='a'&&ch<='z')&&!(ch>='A'&&ch<='Z')&&!(ch>='0'&&ch<='9')&&ch!='_')return false;
 return true;
}
char prefix(PartType type) {
 switch(type){case PartType::Resistor:return 'R';case PartType::Capacitor:return 'C';case PartType::SiliconDiode:return 'D';}
 throw std::invalid_argument("Unknown component type");
}
}
void validateCircuit(const Circuit& circuit) {
 if(circuit.parts.empty()||circuit.parts.size()>10000)throw std::invalid_argument("Circuit must contain 1..10000 parts");
 if(!validToken(circuit.inputNode)||!validToken(circuit.outputNode)||!validToken(circuit.groundNode)||
 circuit.inputNode==circuit.outputNode||circuit.inputNode==circuit.groundNode||circuit.outputNode==circuit.groundNode||
 circuit.groundNode!="0")throw std::invalid_argument("Invalid input/output/ground nodes");
 std::set<std::string> references;
 std::unordered_map<std::string,std::vector<std::string>> links;
 for(const auto& p:circuit.parts){
  if(!validToken(p.reference)||p.reference.front()!=prefix(p.type)||!references.insert(p.reference).second)
   throw std::invalid_argument("Invalid or repeated component reference");
  if(!validToken(p.pin1)||!validToken(p.pin2)||p.pin1==p.pin2)
   throw std::invalid_argument("Invalid or shorted component pins");
  if(!std::isfinite(p.value)||((p.type!=PartType::SiliconDiode)&&(p.value<=0)))
   throw std::invalid_argument("Invalid electrical parameter");
  links[p.pin1].push_back(p.pin2);links[p.pin2].push_back(p.pin1);
 }
 if(!links.contains(circuit.inputNode)||!links.contains(circuit.outputNode)||!links.contains(circuit.groundNode))
   throw std::invalid_argument("Missing an input/output/ground connection");
 std::queue<std::string> pending;std::unordered_set<std::string> reached;
 pending.push(circuit.inputNode);reached.insert(circuit.inputNode);
 while(!pending.empty()){
  const auto current=pending.front();pending.pop();
  for(const auto& next:links.at(current))if(reached.insert(next).second)pending.push(next);
 }
 if(reached.size()!=links.size()||!reached.contains(circuit.outputNode)||!reached.contains(circuit.groundNode))
   throw std::invalid_argument("Disconnected or floating component network");
}
std::string generateSpiceElements(const Circuit& circuit) {
 validateCircuit(circuit);
 std::ostringstream out;out<<std::setprecision(17);
 bool diode=false;
 for(const auto& p:circuit.parts) {
  out<<p.reference<<" "<<p.pin1<<" "<<p.pin2<<" ";
  if(p.type==PartType::SiliconDiode){out<<"DGEN";diode=true;}
  else out<<p.value;
  out<<"\n";
 }
 if(diode)out<<".model DGEN D(Is=2.52e-9 N=1.906 Rs=0.568)\n";
 return out.str();
}
Circuit makeRcCircuit(double ohms,double farads,bool diodeClipper){
 Circuit c;
 c.parts.push_back({"R1",PartType::Resistor,"in","out",ohms});
 c.parts.push_back({"C1",PartType::Capacitor,"out","0",farads});
 if(diodeClipper){
  c.parts.push_back({"D1",PartType::SiliconDiode,"out","0",0});
  c.parts.push_back({"D2",PartType::SiliconDiode,"0","out",0});
 }
 return c;
}
}
