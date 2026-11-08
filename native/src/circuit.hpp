#pragma once
#include <string>
#include <vector>
namespace vbal {
enum class PartType { Resistor, Capacitor, Inductor, Switch, DcVoltageSource, NpnTransistor, PnpTransistor, NmosTransistor, NjfetTransistor, OpAmp, Pt2399, Mn3007, Mn3101, SiliconDiode, Led, SchottkyDiode, GermaniumDiode, ZenerDiode };
struct Part {
 std::string reference;
 PartType type;
 std::string pin1;
 std::string pin2;
 double value; // ohms for R, farads for C, henries for L; transistor beta/model value
 std::string pin3{}; // third terminal
 std::string pin4{}; // fourth terminal for multi-pin parts
 std::string pin5{}; // fifth terminal for multi-pin parts
 std::vector<std::string> extraPins{}; // IC pins 6..16 (DIP-16) or 6..8 (DIP-8)
 bool germanium=false; // PNP BJT technology; activates a dedicated Ge model
};
struct Circuit {
 std::vector<Part> parts;
 std::string inputNode="in";
 std::string outputNode="out";
 std::string groundNode="0";
};
void validateCircuit(const Circuit& circuit);
Circuit parseEditorSpiceElements(const std::string& elements);
std::string generateSpiceElements(const Circuit& circuit);
std::string exportCircuitSpice(const Circuit& circuit);
std::string exportCircuitBomCsv(const Circuit& circuit);
std::string exportCircuitWiring(const Circuit& circuit);
Circuit makeRcCircuit(double ohms,double farads,bool diodeClipper);
}
