#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <mutex>
static_assert(std::atomic<float>::is_always_lock_free, "Audio meter requires lock-free float atomics");
#include "FuzzEngine.h"
#include "RoomConvolution.h"
#include "FactoryRoom.h"

class MicrophonicFuzzAudioProcessor final : public juce::AudioProcessor, private juce::AsyncUpdater, private juce::Timer
{
public:
    MicrophonicFuzzAudioProcessor();
    ~MicrophonicFuzzAudioProcessor() override { cancelPendingUpdate(); stopTimer(); }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Microphonic Fuzz"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 60.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState parameters;
    double getProcessingSampleRate() const {return processingRate.load(std::memory_order_relaxed);}
    float getOutputRms() const {return outputRms.load(std::memory_order_relaxed);}
    float getOutputPeak() const { return outputPeak.load(std::memory_order_relaxed); }
    float getOutputPeak(int channel) const {return channelPeaks[static_cast<size_t>(std::clamp(channel,0,1))].load(std::memory_order_relaxed);}
    void copyRoomIRPreview(float* destination,int size) const;
    double getRoomIRDuration() const {return roomIRDuration.load(std::memory_order_relaxed);}
    int copyWaveform(float* destination, int maxSamples) const;
    juce::Result loadUserIR(const juce::File& file);
    bool hasUserIR() const { return room.hasUserIR(); }
    float getMicVisualOffset() const {return micVisualOffset.load(std::memory_order_relaxed);}
    bool isIRPreparing() const {return room.swapPending();}
    juce::String getIRStatus() const {return isIRPreparing()?"Preparing IR...":hasUserIR()?"User IR active":parameters.getRawParameterValue("roomMode")->load()>=.5f?"Synthetic small room":"No user IR";}
    bool hasFactoryIR() const { return true; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    FuzzEngine engine;
    RoomConvolution room;
    std::atomic<int> roomModeApplied{-1};
    std::atomic<float> roomDecayApplied{-1};
    std::atomic<bool> retryIR{false};
    std::atomic<float> outputPeak{0},outputRms{0},micVisualOffset{0};
    std::array<std::atomic<float>,2> channelPeaks{};
    std::array<std::atomic<float>,128> roomIRPreview{};
    std::atomic<double> roomIRDuration{0};
    void updateRoomIRPreview(const juce::AudioBuffer<float>* impulse,double rate);
    std::array<std::atomic<float>,512> waveform{};
    std::atomic<unsigned> waveformPosition{0};
    std::atomic<double> processingRate{44100};
    std::mutex irLoadingMutex;
    void handleAsyncUpdate() override;
    void timerCallback() override;
    juce::Result restoreEmbeddedIR();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrophonicFuzzAudioProcessor)
};
