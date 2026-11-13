#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

MicrophonicFuzzAudioProcessor::MicrophonicFuzzAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "Parameters", makeLayout())
{}

juce::AudioProcessorValueTreeState::ParameterLayout MicrophonicFuzzAudioProcessor::makeLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&p] (const char* id, const char* name, float min, float max, float def)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (min, max), def));
    };
    add ("gain", "Fuzz Gain", 1.0f, 30.0f, 6.0f);
    add ("feedback", "Feedback", 0.0f, 0.85f, 0.2f);
    add ("mix", "Mix", 0.0f, 1.0f, 1.0f);
    add ("output", "Output", 0.0f, 2.0f, 0.8f);
    return { p.begin(), p.end() };
}

void MicrophonicFuzzAudioProcessor::prepareToPlay (double, int)
{
    feedbackState.fill (0.0f);
}

void MicrophonicFuzzAudioProcessor::releaseResources() {}

void MicrophonicFuzzAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto gain = parameters.getRawParameterValue ("gain")->load();
    const auto feedback = parameters.getRawParameterValue ("feedback")->load();
    const auto mix = parameters.getRawParameterValue ("mix")->load();
    const auto output = parameters.getRawParameterValue ("output")->load();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        float previous = feedbackState[static_cast<size_t> (ch)];
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float wet = std::tanh (gain * (dry + feedback * previous));
            previous = wet;
            data[i] = output * ((1.0f - mix) * dry + mix * wet);
        }
        feedbackState[static_cast<size_t> (ch)] = previous;
    }
}

void MicrophonicFuzzAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void MicrophonicFuzzAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* MicrophonicFuzzAudioProcessor::createEditor()
{
    return new MicrophonicFuzzAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicrophonicFuzzAudioProcessor();
}
