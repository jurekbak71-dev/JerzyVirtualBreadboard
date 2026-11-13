#include "PluginEditor.h"
#if MF_GUI_ASSETS
#include <BinaryData.h>
#endif

namespace
{
class OriginalRackLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    OriginalRackLookAndFeel()
    {
#if MF_GUI_ASSETS
        const char* names[] = {
            "steel_large_128_png", "steel_warm_128_png",
            "black_anodized_128_png", "steel_bright_128_png"
        };
        for (int i = 0; i < 4; ++i)
        {
            int size = 0;
            if (const char* bytes = BinaryData::getNamedResource (names[i], size))
                filmstrips[i] = juce::ImageFileFormat::loadFrom (bytes, static_cast<size_t> (size));
        }
#endif
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
#if MF_GUI_ASSETS
        const auto* property = slider.getProperties().getVarPointer ("rackKnobIndex");
        const int index = property != nullptr ? static_cast<int> (*property) : 0;
        const auto& sheet = filmstrips[static_cast<size_t> (juce::jlimit (0, 3, index))];
        if (sheet.isValid() && sheet.getWidth() == 1536 && sheet.getHeight() == 768)
        {
            const int frame = juce::jlimit (0, 127,
                juce::roundToInt (sliderPosProportional * 127.0f));
            const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
            const auto side = juce::jmin (bounds.getWidth(), bounds.getHeight());
            const auto dest = juce::Rectangle<float> (side, side).withCentre (bounds.getCentre());
            g.drawImage (sheet, dest.getX(), dest.getY(), dest.getWidth(), dest.getHeight(),
                         (frame % 16) * 96, (frame / 16) * 96, 96, 96);
            return;
        }
#endif
        juce::LookAndFeel_V4::drawRotarySlider (
            g, x, y, width, height, sliderPosProportional,
            rotaryStartAngle, rotaryEndAngle, slider);
    }
private:
#if MF_GUI_ASSETS
    std::array<juce::Image, 4> filmstrips;
#endif
};
OriginalRackLookAndFeel& rackStyle()
{
    static OriginalRackLookAndFeel instance;
    return instance;
}
}

MicrophonicFuzzAudioProcessorEditor::MicrophonicFuzzAudioProcessorEditor (MicrophonicFuzzAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    const std::array<const char*, 4> ids { "gain", "feedback", "mix", "output" };
    const std::array<const char*, 4> names { "FUZZ", "FEEDBACK", "MIX", "OUTPUT" };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& slider = knobs[i];
        slider.setLookAndFeel (&rackStyle());
        slider.getProperties().set ("rackKnobIndex", static_cast<int> (i));
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

MicrophonicFuzzAudioProcessorEditor::~MicrophonicFuzzAudioProcessorEditor()
{
    for (auto& slider : knobs)
        slider.setLookAndFeel (nullptr);
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
