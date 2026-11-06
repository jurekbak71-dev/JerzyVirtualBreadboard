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
 switch(type){case PartType::Resistor:return 'R';case PartType::Capacitor:return 'C';case PartType::Inductor:return 'L';case PartType::Switch:return 'R';case PartType::DcVoltageSource:return 'V';case PartType::NpnTransistor:return 'Q';case PartType::SiliconDiode:return 'D';case PartType::Led:return 'D';case PartType::SchottkyDiode:return 'D';case PartType::GermaniumDiode:return 'D';case PartType::ZenerDiode:return 'D';}
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
  const bool threePin=p.type==PartType::NpnTransistor;
  if(!validToken(p.pin1)||!validToken(p.pin2)||p.pin1==p.pin2|| (threePin&&(!validToken(p.pin3)||p.pin3==p.pin1||p.pin3==p.pin2)))
   throw std::invalid_argument("Invalid or shorted component pins");
  const bool diodeFamily=p.type==PartType::SiliconDiode||p.type==PartType::Led||p.type==PartType::SchottkyDiode||p.type==PartType::GermaniumDiode||p.type==PartType::ZenerDiode;
  if(!std::isfinite(p.value)||(!diodeFamily&&p.type!=PartType::Switch&&p.type!=PartType::DcVoltageSource&&p.type!=PartType::NpnTransistor&&(p.value<=0))||(p.type==PartType::Switch&&p.value!=0.&&p.value!=1.)||(p.type==PartType::NpnTransistor&&p.value<=1.))
   throw std::invalid_argument("Invalid electrical parameter");
  links[p.pin1].push_back(p.pin2);links[p.pin2].push_back(p.pin1);
  if(threePin){links[p.pin1].push_back(p.pin3);links[p.pin2].push_back(p.pin3);links[p.pin3].push_back(p.pin1);links[p.pin3].push_back(p.pin2);}
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
 bool diode=false,led=false,schottky=false,germanium=false,zener=false;
 bool npn=false;
 for(const auto& p:circuit.parts) {
  if(p.type==PartType::NpnTransistor){
   out<<p.reference<<" "<<p.pin1<<" "<<p.pin2<<" "<<p.pin3<<" NPNGEN\n";npn=true;continue;
  }
  out<<p.reference<<" "<<p.pin1<<" "<<p.pin2<<" ";
  if(p.type==PartType::SiliconDiode){out<<"DGEN";diode=true;}
  else if(p.type==PartType::Led){out<<"LEDGEN";led=true;}
  else if(p.type==PartType::SchottkyDiode){out<<"SCHGEN";schottky=true;}
  else if(p.type==PartType::GermaniumDiode){out<<"GEGEN";germanium=true;}
  else if(p.type==PartType::ZenerDiode){out<<"Z5V1";zener=true;}
  else if(p.type==PartType::Switch)out<<(p.value>0.5?0.01:1e12);
  else out<<p.value;
  out<<"\n";
 }
 if(diode)out<<".model DGEN D(Is=2.52e-9 N=1.906 Rs=0.568)\n";
 if(led)out<<".model LEDGEN D(Is=1e-20 N=2 Rs=10)\n";
 if(schottky)out<<".model SCHGEN D(Is=1e-7 N=1.05 Rs=0.2)\n";
 if(germanium)out<<".model GEGEN D(Is=1e-6 N=1.1 Rs=1)\n";
 if(zener)out<<".model Z5V1 D(Is=2.52e-9 N=1.906 Rs=0.568 BV=5.1 IBV=1m)\n";
 if(npn)out<<".model NPNGEN NPN(Is=1e-15 Bf=100 Vaf=100)\n";
 return out.str();
}

Circuit parseEditorSpiceElements(const std::string& elements){
 if(elements.size()>65536)throw std::invalid_argument("Circuit definition too large");
 std::istringstream source(elements);std::string line;Circuit graph;
 bool modelSeen=false,ledModelSeen=false,schModelSeen=false,geModelSeen=false,zModelSeen=false,npnModelSeen=false,anyDiode=false,anyLed=false,anySch=false,anyGe=false,anyZ=false,anyNpn=false;size_t lineNo=0;
 while(std::getline(source,line)){
  ++lineNo;
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty())continue;
  if(line.rfind(".model ",0)==0){
   if(line==".model DGEN D(Is=2.52e-9 N=1.906 Rs=0.568)"){if(modelSeen)throw std::invalid_argument("Repeated diode model");modelSeen=true;continue;}
   if(line==".model LEDGEN D(Is=1e-20 N=2 Rs=10)"){if(ledModelSeen)throw std::invalid_argument("Repeated LED model");ledModelSeen=true;continue;}
   if(line==".model SCHGEN D(Is=1e-7 N=1.05 Rs=0.2)"){if(schModelSeen)throw std::invalid_argument("Repeated Schottky model");schModelSeen=true;continue;}
   if(line==".model GEGEN D(Is=1e-6 N=1.1 Rs=1)"){if(geModelSeen)throw std::invalid_argument("Repeated germanium model");geModelSeen=true;continue;}
   if(line==".model Z5V1 D(Is=2.52e-9 N=1.906 Rs=0.568 BV=5.1 IBV=1m)"){if(zModelSeen)throw std::invalid_argument("Repeated Zener model");zModelSeen=true;continue;}
   if(line==".model NPNGEN NPN(Is=1e-15 Bf=100 Vaf=100)"){if(npnModelSeen)throw std::invalid_argument("Repeated NPN model");npnModelSeen=true;continue;}
   throw std::invalid_argument("Unsupported SPICE model");
  }
  if(graph.parts.size()>=256)throw std::invalid_argument("Too many elements");
  std::istringstream row(line);std::string name,a,b,value,extra;
  if(!(row>>name))throw std::invalid_argument("Malformed component line");
  if(name[0]=='Q'){
   std::string c,model;
   if(!(row>>a>>b>>c>>model)||(row>>extra))throw std::invalid_argument("Malformed transistor line");
   if(!validToken(name)||!validToken(a)||!validToken(b)||!validToken(c)||model!="NPNGEN")throw std::invalid_argument("Illegal transistor definition");
   graph.parts.push_back({name,PartType::NpnTransistor,a,b,100.,c});anyNpn=true;continue;
  }
  if(!(row>>a>>b>>value)||(row>>extra))throw std::invalid_argument("Malformed component line");
  if(!validToken(name)||!validToken(a)||!validToken(b))throw std::invalid_argument("Illegal element identifier");
  PartType type;
  if(name.rfind("RSW",0)==0)type=PartType::Switch;
  else if(name[0]=='R')type=PartType::Resistor;
  else if(name[0]=='C')type=PartType::Capacitor;
  else if(name[0]=='L')type=PartType::Inductor;
  else if(name[0]=='V')type=PartType::DcVoltageSource;
  else if(name[0]=='D'){
   if(value=="LEDGEN")type=PartType::Led;
   else if(value=="SCHGEN")type=PartType::SchottkyDiode;
   else if(value=="GEGEN")type=PartType::GermaniumDiode;
   else if(value=="Z5V1")type=PartType::ZenerDiode;
   else type=PartType::SiliconDiode;
  }
  else throw std::invalid_argument("Only R, C, L, D and LED components are supported");
  double numeric=0;
  if(type==PartType::SiliconDiode){
   if(value!="DGEN")throw std::invalid_argument("Unknown diode model");
   anyDiode=true;
  }else if(type==PartType::Led){
   if(value!="LEDGEN")throw std::invalid_argument("Unknown LED model");anyLed=true;
  }else if(type==PartType::SchottkyDiode){
   if(value!="SCHGEN")throw std::invalid_argument("Unknown Schottky model");anySch=true;
  }else if(type==PartType::GermaniumDiode){
   if(value!="GEGEN")throw std::invalid_argument("Unknown germanium model");anyGe=true;
  }else if(type==PartType::ZenerDiode){
   if(value!="Z5V1")throw std::invalid_argument("Unknown Zener model");anyZ=true;
  }else{
   size_t used=0;
   try{numeric=std::stod(value,&used);}catch(...){throw std::invalid_argument("Invalid numeric value");}
   if(used!=value.size()||!std::isfinite(numeric)||(type!=PartType::DcVoltageSource&&numeric<=0))throw std::invalid_argument("Invalid component value");
   if(type==PartType::Switch)numeric=numeric<1.?1.:0.;
  }
  graph.parts.push_back({name,type,a,b,numeric});
 }
 if(modelSeen!=anyDiode)throw std::invalid_argument("Missing or redundant diode model");
 if(ledModelSeen!=anyLed)throw std::invalid_argument("Missing or redundant LED model");
 if(schModelSeen!=anySch)throw std::invalid_argument("Missing or redundant Schottky model");
 if(geModelSeen!=anyGe)throw std::invalid_argument("Missing or redundant germanium model");
 if(zModelSeen!=anyZ)throw std::invalid_argument("Missing or redundant Zener model");
 if(npnModelSeen!=anyNpn)throw std::invalid_argument("Missing or redundant NPN model");
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
  const char* type=part.type==PartType::Resistor?"Resistor":part.type==PartType::Capacitor?"Capacitor":part.type==PartType::Inductor?"Inductor":part.type==PartType::Switch?"Switch":part.type==PartType::DcVoltageSource?"DCVoltageSource":part.type==PartType::NpnTransistor?"NPN":part.type==PartType::Led?"LED":part.type==PartType::SchottkyDiode?"SchottkyDiode":part.type==PartType::GermaniumDiode?"GermaniumDiode":part.type==PartType::ZenerDiode?"Zener5V1":"SiliconDiode";
  const char* unit=part.type==PartType::Resistor?"ohm":part.type==PartType::Capacitor?"F":part.type==PartType::Inductor?"H":part.type==PartType::Switch?"state":part.type==PartType::DcVoltageSource?"V":part.type==PartType::NpnTransistor?"beta":part.type==PartType::Led?"model:LEDGEN":part.type==PartType::SchottkyDiode?"model:SCHGEN":part.type==PartType::GermaniumDiode?"model:GEGEN":part.type==PartType::ZenerDiode?"model:Z5V1":"model:DGEN";
  s<<part.reference<<","<<type<<",";
  const bool diodeFamily=part.type==PartType::SiliconDiode||part.type==PartType::Led||part.type==PartType::SchottkyDiode||part.type==PartType::GermaniumDiode||part.type==PartType::ZenerDiode;
  if(!diodeFamily)s<<(part.type==PartType::Switch?(part.value>0.5?1:0):part.value);
  s<<","<<unit<<","<<part.pin1<<","<<part.pin2;
  if(part.type==PartType::NpnTransistor)s<<"/"<<part.pin3;
  s<<"\n";
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
  if(p.type==PartType::NpnTransistor)nets[p.pin3].push_back(p.reference+".3");
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
