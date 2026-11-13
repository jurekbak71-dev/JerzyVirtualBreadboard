#pragma once
#include "PluginProcessor.h"
#include <array>

class MicrophonicFuzzAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MicrophonicFuzzAudioProcessorEditor (MicrophonicFuzzAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    MicrophonicFuzzAudioProcessor& processor;
    std::array<juce::Slider, 4> knobs;
    std::array<juce::Label, 4> labels;
    std::array<std::unique_ptr<Attachment>, 4> attachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrophonicFuzzAudioProcessorEditor)
};
