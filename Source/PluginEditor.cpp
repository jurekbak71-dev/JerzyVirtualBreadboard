#include "PluginEditor.h"

MicrophonicFuzzAudioProcessorEditor::MicrophonicFuzzAudioProcessorEditor (MicrophonicFuzzAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    const std::array<const char*, 4> ids { "gain", "feedback", "mix", "output" };
    const std::array<const char*, 4> names { "FUZZ", "FEEDBACK", "MIX", "OUTPUT" };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& slider = knobs[i];
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 85, 22);
        addAndMakeVisible (slider);
        labels[i].setText (names[i], juce::dontSendNotification);
        labels[i].setJustificationType (juce::Justification::centred);
        addAndMakeVisible (labels[i]);
        attachments[i] = std::make_unique<Attachment> (processor.parameters, ids[i], slider);
    }
    setResizable (true, true);
    setResizeLimits (420, 210, 1200, 650);
    setSize (720, 280);
}

void MicrophonicFuzzAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (28, 31, 35));
    g.setColour (juce::Colours::orange);
    g.setFont (juce::FontOptions (25.0f, juce::Font::bold));
    g.drawText ("MICROPHONIC FUZZ", getLocalBounds().removeFromTop (65),
                juce::Justification::centred);
}

void MicrophonicFuzzAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (65);
    const int width = area.getWidth() / static_cast<int> (knobs.size());
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto cell = area.removeFromLeft (width).reduced (6);
        labels[i].setBounds (cell.removeFromTop (24));
        knobs[i].setBounds (cell);
    }
}
