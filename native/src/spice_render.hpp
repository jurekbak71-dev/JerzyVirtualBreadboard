#pragma once
#include "audio.hpp"
#include <cstddef>
#include <string>
#include <vector>
namespace vbal {
struct SpicePoint { double time; double voltage; };
std::vector<SpicePoint> parseSpiceWrdata(const std::string& text);
std::vector<float> resampleSpiceOutput(const std::vector<SpicePoint>& points, unsigned sampleRate, std::size_t frames);
// Offline ngspice executable from PATH. Not intended for real-time audio.
Audio renderRcWithNgspice(const Audio& input, double resistanceOhms, double capacitanceFarads);
}
