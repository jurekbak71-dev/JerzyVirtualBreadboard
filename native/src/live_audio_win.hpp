#pragma once
#ifdef _WIN32
#include "circuit.hpp"
#include <atomic>
#include <memory>
#include <string>
struct ma_device;
namespace vbal {
class RealtimeCircuitProcessor;
class LiveAudioEngine {
public:
 LiveAudioEngine();
 ~LiveAudioEngine();
 LiveAudioEngine(const LiveAudioEngine&)=delete;
 LiveAudioEngine& operator=(const LiveAudioEngine&)=delete;
 void start(const Circuit& circuit);
 void stop() noexcept;
 bool running() const noexcept { return running_.load(); }
 bool faulted() const noexcept { return faulted_.load(); }
 float inputPeak() const noexcept { return inputPeak_.load(); }
 float outputPeak() const noexcept { return outputPeak_.load(); }
 unsigned sampleRate() const noexcept { return sampleRate_; }
 unsigned channels() const noexcept { return channels_; }
 std::string backendName() const;
private:
 struct Impl;
 std::unique_ptr<Impl> impl_;
 std::unique_ptr<RealtimeCircuitProcessor> left_,right_;
 std::atomic<bool> running_{false},faulted_{false};
 std::atomic<float> inputPeak_{0.f},outputPeak_{0.f};
 unsigned sampleRate_{48000},channels_{2};
 static void dataCallback(ma_device* device,void* output,const void* input,unsigned frameCount);
 void process(float* output,const float* input,unsigned frameCount) noexcept;
};
}
#endif
