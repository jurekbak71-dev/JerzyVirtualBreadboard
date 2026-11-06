#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
namespace vbal {
struct Audio { uint32_t sampleRate{}; uint16_t channels{}; std::vector<float> samples; };
Audio readPcmWav(const std::filesystem::path& path);
void writePcm16Wav(const std::filesystem::path& path, const Audio& audio);
void rcLowpass(Audio& audio, double resistanceOhms, double capacitanceFarads);
}
