#pragma once
#include <string>
#include <vector>
namespace vbal {
enum class PartType { Resistor, Capacitor, Inductor, Switch, DcVoltageSource, NpnTransistor, PnpTransistor, NmosTransistor, NjfetTransistor, SiliconDiode, Led, SchottkyDiode, GermaniumDiode, ZenerDiode };
struct Part {
 std::string reference;
 PartType type;
 std::string pin1;
 std::string pin2;
 double value; // ohms for R, farads for C, henries for L; transistor beta/model value
 std::string pin3{}; // third terminal for active multi-pin parts (e.g. transistor emitter)
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
