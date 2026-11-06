#include "circuit.hpp"
#include <cmath>
#include <map>
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

Circuit parseEditorSpiceElements(const std::string& elements){
 if(elements.size()>65536)throw std::invalid_argument("Circuit definition too large");
 std::istringstream source(elements);std::string line;Circuit graph;
 bool modelSeen=false,anyDiode=false;size_t lineNo=0;
 while(std::getline(source,line)){
  ++lineNo;
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty())continue;
  if(line.rfind(".model ",0)==0){
   if(modelSeen || line!=".model DGEN D(Is=2.52e-9 N=1.906 Rs=0.568)")
    throw std::invalid_argument("Unsupported SPICE model");
   modelSeen=true;continue;
  }
  if(graph.parts.size()>=256)throw std::invalid_argument("Too many elements");
  std::istringstream row(line);std::string name,a,b,value,extra;
  if(!(row>>name>>a>>b>>value)||(row>>extra))throw std::invalid_argument("Malformed component line");
  if(!validToken(name)||!validToken(a)||!validToken(b))throw std::invalid_argument("Illegal element identifier");
  PartType type;
  if(name[0]=='R')type=PartType::Resistor;
  else if(name[0]=='C')type=PartType::Capacitor;
  else if(name[0]=='D')type=PartType::SiliconDiode;
  else throw std::invalid_argument("Only R, C and D components are supported");
  double numeric=0;
  if(type==PartType::SiliconDiode){
   if(value!="DGEN")throw std::invalid_argument("Unknown diode model");
   anyDiode=true;
  }else{
   size_t used=0;
   try{numeric=std::stod(value,&used);}catch(...){throw std::invalid_argument("Invalid numeric value");}
   if(used!=value.size()||!std::isfinite(numeric)||numeric<=0)
    throw std::invalid_argument("Invalid component value");
  }
  graph.parts.push_back({name,type,a,b,numeric});
 }
 if(modelSeen!=anyDiode)throw std::invalid_argument("Missing or redundant diode model");
 validateCircuit(graph);
 return graph;
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

std::string exportCircuitSpice(const Circuit& circuit){
 const auto elements=generateSpiceElements(circuit);
 std::ostringstream s;s<<std::setprecision(17);
 s<<"* VBAL physical circuit topology — simulated input 1 kHz test signal\n";
 s<<"Vin "<<circuit.inputNode<<" "<<circuit.groundNode<<" SIN(0 1 1000) AC 1\n";
 s<<elements;
 s<<".control\ntran 1u 10m\n.endc\n.end\n";
 return s.str();
}
std::string exportCircuitBomCsv(const Circuit& circuit){
 validateCircuit(circuit);
 std::ostringstream s;s<<std::setprecision(17);
 s<<"Reference,Type,Value,Unit,Pin1,Pin2\n";
 for(const auto& part:circuit.parts){
  const char* type=part.type==PartType::Resistor?"Resistor":part.type==PartType::Capacitor?"Capacitor":"SiliconDiode";
  const char* unit=part.type==PartType::Resistor?"ohm":part.type==PartType::Capacitor?"F":"model:DGEN";
  s<<part.reference<<","<<type<<",";
  if(part.type!=PartType::SiliconDiode)s<<part.value;
  s<<","<<unit<<","<<part.pin1<<","<<part.pin2<<"\n";
 }
 return s.str();
}
std::string exportCircuitWiring(const Circuit& circuit){
 validateCircuit(circuit);
 std::ostringstream out;
 out<<"# VBAL — net connections (logical, not physical hole locations)\n";
 out<<"# Connect all listed component terminals sharing the same net.\n";
 out<<"# No power supply, pinout, layout or safety approval inferred.\n";
 std::map<std::string,std::vector<std::string>> nets;
 for(const auto& p:circuit.parts){
  nets[p.pin1].push_back(p.reference+".1");
  nets[p.pin2].push_back(p.reference+".2");
 }
 nets[circuit.inputNode].push_back("INPUT.signal");
 nets[circuit.outputNode].push_back("OUTPUT.signal");
 nets[circuit.groundNode].push_back("INPUT.ground");
 nets[circuit.groundNode].push_back("OUTPUT.ground");
 for(const auto& [net,pins]:nets){
  out<<net<<": ";
  for(size_t i=0;i<pins.size();++i){if(i)out<<" -- ";out<<pins[i];}
  out<<"\n";
 }
 return out.str();
}

}
