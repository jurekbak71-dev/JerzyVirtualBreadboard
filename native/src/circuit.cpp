#include "circuit.hpp"
#include <algorithm>
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
 switch(type){case PartType::Resistor:return 'R';case PartType::Capacitor:return 'C';case PartType::Inductor:return 'L';case PartType::Switch:return 'R';case PartType::DcVoltageSource:return 'V';case PartType::NpnTransistor:return 'Q';case PartType::PnpTransistor:return 'Q';case PartType::NmosTransistor:return 'M';case PartType::NjfetTransistor:return 'J';case PartType::OpAmp:return 'B';case PartType::Pt2399:return 'X';case PartType::Mn3007:return 'X';case PartType::Mn3101:return 'X';case PartType::SiliconDiode:return 'D';case PartType::Led:return 'D';case PartType::SchottkyDiode:return 'D';case PartType::GermaniumDiode:return 'D';case PartType::ZenerDiode:return 'D';}
 throw std::invalid_argument("Unknown component type");
}
}
void validateCircuit(const Circuit& circuit) {
 if(circuit.parts.empty()||circuit.parts.size()>10000)throw std::invalid_argument("Circuit must contain 1..10000 parts");
 if(!validToken(circuit.inputNode)||!validToken(circuit.outputNode)||!validToken(circuit.groundNode)||
 circuit.inputNode==circuit.outputNode||circuit.inputNode==circuit.groundNode||circuit.outputNode==circuit.groundNode||
 circuit.groundNode!="0")throw std::invalid_argument("Invalid input/output/ground nodes");
 const bool active=std::any_of(circuit.parts.begin(),circuit.parts.end(),[](const Part& p){
  return p.type==PartType::NpnTransistor||p.type==PartType::PnpTransistor||p.type==PartType::NmosTransistor||
         p.type==PartType::NjfetTransistor||p.type==PartType::OpAmp||p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101;
 });
 if(active&&!std::any_of(circuit.parts.begin(),circuit.parts.end(),[](const Part& p){return p.type==PartType::DcVoltageSource;}))
  throw std::invalid_argument("Active transistor or opamp circuit has no power source");
 std::set<std::string> references;
 std::unordered_map<std::string,std::vector<std::string>> links;
 for(const auto& p:circuit.parts){
  if(!validToken(p.reference)||p.reference.front()!=prefix(p.type)||!references.insert(p.reference).second)
   throw std::invalid_argument("Invalid or repeated component reference");
  const bool threePin=p.type==PartType::NpnTransistor||p.type==PartType::PnpTransistor||p.type==PartType::NmosTransistor||p.type==PartType::NjfetTransistor;
  const bool fivePin=p.type==PartType::OpAmp;
  const bool sixteenPin=p.type==PartType::Pt2399;
  const bool eightPin=p.type==PartType::Mn3007||p.type==PartType::Mn3101;
  const bool multiPin=sixteenPin||eightPin;
  if(!validToken(p.pin1)||!validToken(p.pin2)||(p.pin1==p.pin2&&!multiPin)||
    (threePin&&(!validToken(p.pin3)||p.pin3==p.pin1||p.pin3==p.pin2))||
    (fivePin&&(!validToken(p.pin3)||!validToken(p.pin4)||!validToken(p.pin5)||
      p.pin3==p.pin1||p.pin3==p.pin2||p.pin4==p.pin1||p.pin4==p.pin2||p.pin4==p.pin3||
      p.pin5==p.pin1||p.pin5==p.pin2||p.pin5==p.pin3||p.pin5==p.pin4)))
   throw std::invalid_argument("Invalid or shorted component pins");
  if(multiPin){
   if(p.extraPins.size()!=(sixteenPin?11u:3u)||!validToken(p.pin3)||!validToken(p.pin4)||!validToken(p.pin5))
    throw std::invalid_argument("PT2399 requires exactly 16 physical pins");
   for(const auto& pin:p.extraPins)if(!validToken(pin))throw std::invalid_argument("Invalid PT2399 node");
  }
  const bool diodeFamily=p.type==PartType::SiliconDiode||p.type==PartType::Led||p.type==PartType::SchottkyDiode||p.type==PartType::GermaniumDiode||p.type==PartType::ZenerDiode;
  if(!std::isfinite(p.value)||(!diodeFamily&&p.type!=PartType::Switch&&p.type!=PartType::DcVoltageSource&&p.type!=PartType::NpnTransistor&&p.type!=PartType::PnpTransistor&&p.type!=PartType::NmosTransistor&&p.type!=PartType::NjfetTransistor&&p.type!=PartType::OpAmp&&!multiPin&&(p.value<=0))||(p.type==PartType::Switch&&p.value!=0.&&p.value!=1.)||((p.type==PartType::NpnTransistor||p.type==PartType::PnpTransistor)&&p.value<=1.)||((p.type==PartType::NmosTransistor||p.type==PartType::NjfetTransistor)&&p.value<=0.)||(p.type==PartType::OpAmp&&p.value<=10.))
   throw std::invalid_argument("Invalid electrical parameter");
  links[p.pin1].push_back(p.pin2);links[p.pin2].push_back(p.pin1);
  if(threePin){links[p.pin1].push_back(p.pin3);links[p.pin2].push_back(p.pin3);links[p.pin3].push_back(p.pin1);links[p.pin3].push_back(p.pin2);}
  if(multiPin){
   const std::vector<std::string> pins={p.pin1,p.pin2,p.pin3,p.pin4,p.pin5};
   std::vector<std::string> all=pins;
   all.insert(all.end(),p.extraPins.begin(),p.extraPins.end());
   for(size_t i=0;i<all.size();++i)for(size_t j=i+1;j<all.size();++j){
    links[all[i]].push_back(all[j]);links[all[j]].push_back(all[i]);
   }
  }
  if(fivePin){
   const std::string pins[5]={p.pin1,p.pin2,p.pin3,p.pin4,p.pin5};
   for(int i=0;i<5;++i)for(int j=i+1;j<5;++j){links[pins[i]].push_back(pins[j]);links[pins[j]].push_back(pins[i]);}
  }
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
 bool npn=false,pnp=false,nmos=false,njfet=false;
 for(const auto& p:circuit.parts) {
  if(p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101){
   // This is a VBAL mixed-signal extension, NOT a standard ngspice subcircuit.
   // Refuse misleading standalone SPICE output via exportCircuitSpice().
   out<<p.reference<<" "<<p.pin1<<" "<<p.pin2<<" "<<p.pin3<<" "<<p.pin4<<" "<<p.pin5;
   for(const auto& pin:p.extraPins)out<<" "<<pin;
   out<<" "<<(p.type==PartType::Pt2399?"PT2399_VBAL":p.type==PartType::Mn3007?"MN3007_VBAL":"MN3101_VBAL")<<"\n";
   continue;
  }
  if(p.type==PartType::NpnTransistor){
   out<<p.reference<<" "<<p.pin1<<" "<<p.pin2<<" "<<p.pin3<<" "<<(p.value==100.?"NPNGEN":"NPN_"+p.reference)<<"\n";npn=true;continue;
  }
  if(p.type==PartType::PnpTransistor){
   out<<p.reference<<" "<<p.pin1<<" "<<p.pin2<<" "<<p.pin3<<" "<<(p.germanium?"GPNP_"+p.reference:(p.value==100.?"PNPGEN":"PNP_"+p.reference))<<"\n";pnp=true;continue;
  }
  if(p.type==PartType::NmosTransistor){
   out<<p.reference<<" "<<p.pin1<<" "<<p.pin2<<" "<<p.pin3<<" "<<(p.value==2.?"NMOSGEN":"NMOS_"+p.reference)<<"\n";nmos=true;continue;
  }
  if(p.type==PartType::NjfetTransistor){
   out<<p.reference<<" "<<p.pin1<<" "<<p.pin2<<" "<<p.pin3<<" "<<(p.value==2.?"NJFETGEN":"NJFET_"+p.reference)<<"\n";njfet=true;continue;
  }
  if(p.type==PartType::OpAmp){
   out<<p.reference<<" "<<p.pin1<<" 0 V=max(V("<<p.pin5<<")+0.2,min(V("<<p.pin4<<")-0.2,"<<p.value<<"*(V("<<p.pin2<<")-V("<<p.pin3<<"))))\n";
   continue;
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
 if(std::any_of(circuit.parts.begin(),circuit.parts.end(),[](const Part& p){return p.type==PartType::NpnTransistor&&p.value==100.;}))out<<".model NPNGEN NPN(Is=1e-15 Bf=100 Vaf=100)\n";
 if(std::any_of(circuit.parts.begin(),circuit.parts.end(),[](const Part& p){return p.type==PartType::PnpTransistor&&!p.germanium&&p.value==100.;}))out<<".model PNPGEN PNP(Is=1e-15 Bf=100 Vaf=100)\n";
 for(const auto& p:circuit.parts){
  if(p.type==PartType::NpnTransistor&&p.value!=100.)out<<".model NPN_"<<p.reference<<" NPN(Is=1e-15 Bf="<<p.value<<" Vaf=100)\n";
  else if(p.type==PartType::PnpTransistor&&p.germanium)out<<".model GPNP_"<<p.reference<<" PNP(Is=5e-6 Bf="<<p.value<<" Vaf=40 Eg=0.72)\n";
  else if(p.type==PartType::PnpTransistor&&p.value!=100.)out<<".model PNP_"<<p.reference<<" PNP(Is=1e-15 Bf="<<p.value<<" Vaf=100)\n";
 }
 if(std::any_of(circuit.parts.begin(),circuit.parts.end(),[](const Part& p){return p.type==PartType::NmosTransistor&&p.value==2.;}))out<<".model NMOSGEN NMOS(VTO=2 KP=0.02 LAMBDA=0.02)\n";
 if(std::any_of(circuit.parts.begin(),circuit.parts.end(),[](const Part& p){return p.type==PartType::NjfetTransistor&&p.value==2.;}))out<<".model NJFETGEN NJF(BETA=0.002 VTO=-2 LAMBDA=0.02)\n";
 for(const auto& p:circuit.parts){
  if(p.type==PartType::NmosTransistor&&p.value!=2.)out<<".model NMOS_"<<p.reference<<" NMOS(VTO="<<p.value<<" KP=0.02 LAMBDA=0.02)\n";
  else if(p.type==PartType::NjfetTransistor&&p.value!=2.)out<<".model NJFET_"<<p.reference<<" NJF(BETA=0.002 VTO=-"<<p.value<<" LAMBDA=0.02)\n";
 }
 return out.str();
}

Circuit parseEditorSpiceElements(const std::string& elements){
 if(elements.size()>65536)throw std::invalid_argument("Circuit definition too large");
 std::istringstream source(elements);std::string line;Circuit graph;
 std::unordered_map<std::string,double> customNpn,customPnp,customNmos,customNjfet;
 bool modelSeen=false,ledModelSeen=false,schModelSeen=false,geModelSeen=false,zModelSeen=false,npnModelSeen=false,pnpModelSeen=false,nmosModelSeen=false,njfetModelSeen=false,anyDiode=false,anyLed=false,anySch=false,anyGe=false,anyZ=false,anyNpn=false,anyPnp=false,anyNmos=false,anyNjfet=false;size_t lineNo=0;
 while(std::getline(source,line)){
  ++lineNo;
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty())continue;
  if(line.rfind(".model ",0)==0){

   const std::regex mosModel(R"(^\.model NMOS_([A-Za-z0-9_]+) NMOS\(VTO=([0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?) KP=0.02 LAMBDA=0.02\)$)");
   const std::regex jfetModel(R"(^\.model NJFET_([A-Za-z0-9_]+) NJF\(BETA=0.002 VTO=-([0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?) LAMBDA=0.02\)$)");
   std::smatch transistorMatch;
   if(std::regex_match(line,transistorMatch,mosModel)||std::regex_match(line,transistorMatch,jfetModel)){
    const bool mos=line.rfind(".model NMOS_",0)==0;
    const double value=std::stod(transistorMatch[2].str());
    if(!(value>0.&&value<100.))throw std::invalid_argument("Invalid field-effect transistor model parameter");
    auto& map=mos?customNmos:customNjfet;
    if(!map.emplace((mos?"NMOS_":"NJFET_")+transistorMatch[1].str(),value).second)
     throw std::invalid_argument("Duplicate field-effect transistor model");
    if(mos)nmosModelSeen=true;else njfetModelSeen=true;
    continue;
   }

   const std::regex germaniumPnp(R"(^\.model GPNP_([A-Za-z0-9_]+) PNP\(Is=5e-6 Bf=([0-9]+(?:\.[0-9]+)?) Vaf=40 Eg=0.72\)$)");
   std::smatch geMatch;
   if(std::regex_match(line,geMatch,germaniumPnp)){
    const double beta=std::stod(geMatch[2].str());
    if(!(beta>1.&&beta<1e5))throw std::invalid_argument("Invalid germanium BJT beta");
    if(!customPnp.emplace("GPNP_"+geMatch[1].str(),beta).second)throw std::invalid_argument("Repeated germanium BJT model");
    pnpModelSeen=true;continue;
   }
   const std::regex bjtModel(R"(^\.model (NPN|PNP)_([A-Za-z0-9_]+) (NPN|PNP)\(Is=1e-15 Bf=([0-9]+(?:\.[0-9]+)?) Vaf=100\)$)");
   std::smatch match;
   if(std::regex_match(line,match,bjtModel)&&match[1].str()==match[3].str()){
    const double beta=std::stod(match[4].str());
    if(!(beta>1.&&beta<1e5))throw std::invalid_argument("Invalid transistor beta");
    auto& map=match[1].str()=="NPN"?customNpn:customPnp;
    if(!map.emplace(match[1].str()+"_"+match[2].str(),beta).second)throw std::invalid_argument("Repeated transistor model");
    if(match[1].str()=="NPN")npnModelSeen=true;else pnpModelSeen=true;
    continue;
   }
   if(line==".model DGEN D(Is=2.52e-9 N=1.906 Rs=0.568)"){if(modelSeen)throw std::invalid_argument("Repeated diode model");modelSeen=true;continue;}
   if(line==".model LEDGEN D(Is=1e-20 N=2 Rs=10)"){if(ledModelSeen)throw std::invalid_argument("Repeated LED model");ledModelSeen=true;continue;}
   if(line==".model SCHGEN D(Is=1e-7 N=1.05 Rs=0.2)"){if(schModelSeen)throw std::invalid_argument("Repeated Schottky model");schModelSeen=true;continue;}
   if(line==".model GEGEN D(Is=1e-6 N=1.1 Rs=1)"){if(geModelSeen)throw std::invalid_argument("Repeated germanium model");geModelSeen=true;continue;}
   if(line==".model Z5V1 D(Is=2.52e-9 N=1.906 Rs=0.568 BV=5.1 IBV=1m)"){if(zModelSeen)throw std::invalid_argument("Repeated Zener model");zModelSeen=true;continue;}
   if(line==".model NPNGEN NPN(Is=1e-15 Bf=100 Vaf=100)"){if(npnModelSeen)throw std::invalid_argument("Repeated NPN model");npnModelSeen=true;continue;}
   if(line==".model PNPGEN PNP(Is=1e-15 Bf=100 Vaf=100)"){if(pnpModelSeen)throw std::invalid_argument("Repeated PNP model");pnpModelSeen=true;continue;}
   if(line==".model NMOSGEN NMOS(VTO=2 KP=0.02 LAMBDA=0.02)"){if(nmosModelSeen)throw std::invalid_argument("Repeated NMOS model");nmosModelSeen=true;continue;}
   if(line==".model NJFETGEN NJF(BETA=0.002 VTO=-2 LAMBDA=0.02)"){if(njfetModelSeen)throw std::invalid_argument("Repeated NJFET model");njfetModelSeen=true;continue;}
   throw std::invalid_argument("Unsupported SPICE model");
  }
  if(graph.parts.size()>=256)throw std::invalid_argument("Too many elements");
  std::istringstream row(line);std::string name,a,b,value,extra;
  if(!(row>>name))throw std::invalid_argument("Malformed component line");
  if(name[0]=='X'){
   if(!validToken(name))throw std::invalid_argument("Invalid IC reference");
   std::vector<std::string> tokens;std::string token;
   while(row>>token)tokens.push_back(token);
   const bool pt=tokens.size()==17&&tokens.back()=="PT2399_VBAL";
   const bool mn=tokens.size()==9&&(tokens.back()=="MN3007_VBAL"||tokens.back()=="MN3101_VBAL");
   if(!pt&&!mn)throw std::invalid_argument("Supported X devices: PT2399 DIP16, MN3007/MN3101 DIP8");
   const size_t count=pt?16:8;
   for(size_t i=0;i<count;++i)if(!validToken(tokens[i]))
    throw std::invalid_argument("Invalid mixed-signal IC pin node");
   const PartType type=pt?PartType::Pt2399:tokens.back()=="MN3007_VBAL"?PartType::Mn3007:PartType::Mn3101;
   Part part{name,type,tokens[0],tokens[1],0.,tokens[2],tokens[3],tokens[4]};
   part.extraPins.assign(tokens.begin()+5,tokens.begin()+count);
   graph.parts.push_back(std::move(part));continue;
  }
  if(name[0]=='Q'){
   std::string c,model;
   if(!(row>>a>>b>>c>>model)||(row>>extra))throw std::invalid_argument("Malformed transistor line");
   if(!validToken(name)||!validToken(a)||!validToken(b)||!validToken(c))throw std::invalid_argument("Illegal transistor definition");
   if(model=="NPNGEN"||model=="NPN_"+name){
    const double beta=model=="NPNGEN"?100.:-1.;
    graph.parts.push_back({name,PartType::NpnTransistor,a,b,beta,c});anyNpn=true;continue;
   }
   if(model=="PNPGEN"||model=="PNP_"+name||model=="GPNP_"+name){
    const double beta=model=="PNPGEN"?100.:-1.;
    graph.parts.push_back({name,PartType::PnpTransistor,a,b,beta,c});
    graph.parts.back().germanium=(model=="GPNP_"+name);
    anyPnp=true;continue;
   }
   throw std::invalid_argument("Unsupported BJT model");
  }
  if(name[0]=='M'){
   std::string source,model;
   if(!(row>>a>>b>>source>>model)||(row>>extra))throw std::invalid_argument("Malformed MOSFET line");
   if(!validToken(name)||!validToken(a)||!validToken(b)||!validToken(source)||(model!="NMOSGEN"&&model!="NMOS_"+name))throw std::invalid_argument("Illegal MOSFET definition");
   graph.parts.push_back({name,PartType::NmosTransistor,a,b,model=="NMOSGEN"?2.:-1.,source});anyNmos=true;continue;
  }
  if(name[0]=='B'){
   std::string outNode,ground,expr;
   if(!(row>>outNode>>ground>>expr)||(row>>extra))throw std::invalid_argument("Malformed op-amp line");
   if(ground!="0"||!validToken(name)||!validToken(outNode))throw std::invalid_argument("Illegal op-amp definition");
   const std::regex re(R"(^V=max\(V\(([A-Za-z0-9_]+)\)\+0\.2,min\(V\(([A-Za-z0-9_]+)\)-0\.2,([0-9]+(?:\.[0-9]+)?)\*\(V\(([A-Za-z0-9_]+)\)-V\(([A-Za-z0-9_]+)\)\)\)\)$)");
   std::smatch m;
   if(!std::regex_match(expr,m,re))throw std::invalid_argument("Unsupported op-amp expression");
   const double gain=std::stod(m[3].str());
   graph.parts.push_back({name,PartType::OpAmp,outNode,m[4].str(),gain,m[5].str(),m[2].str(),m[1].str()});
   continue;
  }
  if(name[0]=='J'){
   std::string source,model;
   if(!(row>>a>>b>>source>>model)||(row>>extra))throw std::invalid_argument("Malformed JFET line");
   if(!validToken(name)||!validToken(a)||!validToken(b)||!validToken(source)||(model!="NJFETGEN"&&model!="NJFET_"+name))throw std::invalid_argument("Illegal JFET definition");
   graph.parts.push_back({name,PartType::NjfetTransistor,a,b,model=="NJFETGEN"?2.:-1.,source});anyNjfet=true;continue;
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
 for(auto& p:graph.parts){
  if(p.type==PartType::NpnTransistor&&p.value<0){
   const std::string model="NPN_"+p.reference;
   if(!customNpn.contains(model))throw std::invalid_argument("Missing custom NPN model");
   p.value=customNpn.at(model);
  }else if(p.type==PartType::PnpTransistor&&p.value<0){
   const std::string model=(p.germanium?"GPNP_":"PNP_")+p.reference;
   if(!customPnp.contains(model))throw std::invalid_argument("Missing custom PNP model");
   p.value=customPnp.at(model);
  }
 }
 for(auto& p:graph.parts){
  if(p.type==PartType::NmosTransistor&&p.value<0){
   const std::string model="NMOS_"+p.reference;
   if(!customNmos.contains(model))throw std::invalid_argument("Missing custom NMOS model");
   p.value=customNmos.at(model);
  }else if(p.type==PartType::NjfetTransistor&&p.value<0){
   const std::string model="NJFET_"+p.reference;
   if(!customNjfet.contains(model))throw std::invalid_argument("Missing custom JFET model");
   p.value=customNjfet.at(model);
  }
 }
 if(modelSeen!=anyDiode)throw std::invalid_argument("Missing or redundant diode model");
 if(ledModelSeen!=anyLed)throw std::invalid_argument("Missing or redundant LED model");
 if(schModelSeen!=anySch)throw std::invalid_argument("Missing or redundant Schottky model");
 if(geModelSeen!=anyGe)throw std::invalid_argument("Missing or redundant germanium model");
 if(zModelSeen!=anyZ)throw std::invalid_argument("Missing or redundant Zener model");
 if(npnModelSeen!=anyNpn)throw std::invalid_argument("Missing or redundant NPN model");
 if(pnpModelSeen!=anyPnp)throw std::invalid_argument("Missing or redundant PNP model");
 if(nmosModelSeen!=anyNmos)throw std::invalid_argument("Missing or redundant NMOS model");
 if(njfetModelSeen!=anyNjfet)throw std::invalid_argument("Missing or redundant NJFET model");
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
 if(std::any_of(circuit.parts.begin(),circuit.parts.end(),[](const Part& p){return p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101;}))
  throw std::invalid_argument("PT2399/MN3007/MN3101 are VBAL mixed-signal components. Standalone ngspice cannot reproduce their internal memory/clock. Use VBAL hybrid renderer.");
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
  const char* type=part.type==PartType::Resistor?"Resistor":part.type==PartType::Capacitor?"Capacitor":part.type==PartType::Inductor?"Inductor":part.type==PartType::Switch?"Switch":part.type==PartType::DcVoltageSource?"DCVoltageSource":part.type==PartType::NpnTransistor?"NPN":part.type==PartType::PnpTransistor?"PNP":part.type==PartType::NmosTransistor?"NMOS":part.type==PartType::NjfetTransistor?"NJFET":part.type==PartType::OpAmp?"OpAmp":part.type==PartType::Pt2399?"PT2399":part.type==PartType::Mn3007?"MN3007":part.type==PartType::Mn3101?"MN3101":part.type==PartType::Led?"LED":part.type==PartType::SchottkyDiode?"SchottkyDiode":part.type==PartType::GermaniumDiode?"GermaniumDiode":part.type==PartType::ZenerDiode?"Zener5V1":"SiliconDiode";
  const char* unit=part.type==PartType::Resistor?"ohm":part.type==PartType::Capacitor?"F":part.type==PartType::Inductor?"H":part.type==PartType::Switch?"state":part.type==PartType::DcVoltageSource?"V":part.type==PartType::NpnTransistor?"beta":part.type==PartType::PnpTransistor?"beta":part.type==PartType::NmosTransistor?"Vth":part.type==PartType::NjfetTransistor?"|Vto|":part.type==PartType::OpAmp?"Aol":part.type==PartType::Pt2399?"DIP16":part.type==PartType::Mn3007||part.type==PartType::Mn3101?"DIP8":part.type==PartType::Led?"model:LEDGEN":part.type==PartType::SchottkyDiode?"model:SCHGEN":part.type==PartType::GermaniumDiode?"model:GEGEN":part.type==PartType::ZenerDiode?"model:Z5V1":"model:DGEN";
  s<<part.reference<<","<<type<<",";
  const bool diodeFamily=part.type==PartType::SiliconDiode||part.type==PartType::Led||part.type==PartType::SchottkyDiode||part.type==PartType::GermaniumDiode||part.type==PartType::ZenerDiode;
  if(!diodeFamily&&part.type!=PartType::Pt2399&&part.type!=PartType::Mn3007&&part.type!=PartType::Mn3101)s<<(part.type==PartType::Switch?(part.value>0.5?1:0):part.value);
  s<<","<<unit<<","<<part.pin1<<","<<part.pin2;
  if(part.type==PartType::NpnTransistor||part.type==PartType::PnpTransistor||part.type==PartType::NmosTransistor||part.type==PartType::NjfetTransistor)s<<"/"<<part.pin3;
  if(part.type==PartType::OpAmp||part.type==PartType::Pt2399||part.type==PartType::Mn3007||part.type==PartType::Mn3101){s<<"/"<<part.pin3<<"/"<<part.pin4<<"/"<<part.pin5;for(const auto& pin:part.extraPins)s<<"/"<<pin;}
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
  if(p.type==PartType::NpnTransistor||p.type==PartType::PnpTransistor||p.type==PartType::NmosTransistor||p.type==PartType::NjfetTransistor)nets[p.pin3].push_back(p.reference+".3");
  if(p.type==PartType::OpAmp||p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101){nets[p.pin3].push_back(p.reference+".3");nets[p.pin4].push_back(p.reference+".4");nets[p.pin5].push_back(p.reference+".5");for(size_t i=0;i<p.extraPins.size();++i)nets[p.extraPins[i]].push_back(p.reference+"."+std::to_string(i+6));}
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
