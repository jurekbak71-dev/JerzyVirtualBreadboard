#pragma once
#include "audio.hpp"
#include <cstddef>
#include <string>
namespace vbal {
// Short-signal ngspice RC reference netlist (<=4096 frames); not a real-time renderer.
std::string makeRcSpiceNetlist(const Audio& audio, double resistanceOhms, double capacitanceFarads, std::size_t channel);
}
