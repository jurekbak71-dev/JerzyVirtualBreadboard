#pragma once
#include "audio.hpp"
#include "circuit.hpp"
#include <cstddef>
#include <string>
#include <vector>
namespace vbal {
struct SpicePoint { double time; double voltage; };
std::vector<SpicePoint> parseSpiceWrdata(const std::string& text);
std::vector<float> resampleSpiceOutput(const std::vector<SpicePoint>& points, unsigned sampleRate, std::size_t frames);
Audio renderGraphWithNgspice(const Audio& input, const Circuit& circuit);
// Dispatches standard circuits to ngspice; mixed-signal IC circuits are computed
// explicitly by the VBAL MNA engine, not falsely advertised as ngspice.
Audio renderGraphWithCompatibleSolver(const Audio& input,const Circuit& circuit);
bool circuitHasMixedSignalIc(const Circuit& circuit) noexcept;
enum class CircuitMode { RcLowpass, DiodeClipper };
Audio renderCircuitWithNgspice(const Audio& input, double resistanceOhms, double capacitanceFarads, CircuitMode mode);
// Offline ngspice executable from PATH. Not intended for real-time audio.
Audio renderRcWithNgspice(const Audio& input, double resistanceOhms, double capacitanceFarads);
}
