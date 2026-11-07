#pragma once
#ifdef _WIN32
#include "circuit.hpp"
#include <atomic>
#include <array>
#include <memory>
#include <filesystem>
#include <string>
#include <vector>
struct ma_device;
namespace vbal {
class RealtimeCircuitProcessor;
struct AudioDeviceLists { std::vector<std::string> inputs; std::vector<std::string> outputs; };
class LiveAudioEngine {
public:
 LiveAudioEngine();
 ~LiveAudioEngine();
 LiveAudioEngine(const LiveAudioEngine&)=delete;
 LiveAudioEngine& operator=(const LiveAudioEngine&)=delete;
 static AudioDeviceLists enumerateDevices();
 void start(const Circuit& circuit,int inputDevice,int outputDevice);
 void startFile(const Circuit& circuit,const std::filesystem::path& path,bool loop,int outputDevice);
 void rewindFile();
 void setFileLoop(bool loop) noexcept;
 void setLevels(float inputGainDb,float driveDb,float outputGainDb,bool bypass,int inputMode) noexcept;
 void setMacroEffect(int mode) noexcept;
 void setProbe(const std::string& node,const std::string& resistorReference="",const std::string& referenceNode="0");
 std::vector<float> scopeTrace() const;
 float probedVoltage() const noexcept {return scopeVoltage_.load(std::memory_order_relaxed);}
 float probedCurrent() const noexcept {return scopeCurrent_.load(std::memory_order_relaxed);}
 bool currentAvailable() const noexcept {return probeResistor_.load(std::memory_order_relaxed)>0.f;}
 void stop() noexcept;
 bool running() const noexcept { return running_.load(); }
 bool faulted() const noexcept { return faulted_.load(); }
 bool fileMode() const noexcept { return fileMode_.load(); }
 bool fileEnded() const noexcept { return fileEnded_.load(); }
 float inputPeak() const noexcept { return inputPeak_.load(); }
 float outputPeak() const noexcept { return outputPeak_.load(); }
 unsigned sampleRate() const noexcept { return sampleRate_; }
 unsigned channels() const noexcept { return channels_; }
 std::string backendName() const;
private:
 struct Impl;
 std::unique_ptr<Impl> impl_;
 std::unique_ptr<RealtimeCircuitProcessor> left_,right_;
 std::atomic<bool> running_{false},faulted_{false},fileMode_{false},fileEnded_{false},fileLoop_{false};
 std::atomic<float> inputPeak_{0.f},outputPeak_{0.f};
 std::atomic<float> inputGain_{1.f},driveGain_{1.f},outputGain_{1.f};
 std::atomic<bool> bypass_{false};
 std::atomic<int> inputMode_{0};
 std::atomic<int> macroEffect_{0};
 Circuit activeCircuit_;
 std::array<std::atomic<float>,256> scopeBuffer_{};
 std::atomic<unsigned> scopeWrite_{0};
 std::atomic<int> probeNode_{-2},probeReferenceNode_{-1},probeResistorA_{-1000000},probeResistorB_{-1000000};
 std::atomic<float> probeResistor_{0.f},scopeVoltage_{0.f},scopeCurrent_{0.f};
 unsigned sampleRate_{48000},channels_{2};
 static void dataCallback(ma_device* device,void* output,const void* input,unsigned frameCount);
 void process(float* output,const float* input,unsigned frameCount) noexcept;
 void processFrames(float* output,const float* input,unsigned frameCount) noexcept;
 void configureProcessors(const Circuit& circuit);
};
}
#endif
