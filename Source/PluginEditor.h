#pragma once
#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

class MicrophonicFuzzAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit MicrophonicFuzzAudioProcessorEditor (MicrophonicFuzzAudioProcessor&);
    ~MicrophonicFuzzAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void setAnalyzerMode(bool vu) {showVU=vu;repaint();}
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Knob
    {
        juce::Rectangle<float> bounds;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<SliderAttachment> attachment;
    };
    struct Switch
    {
        juce::Rectangle<float> bounds;
        std::unique_ptr<juce::ToggleButton> button;
        std::unique_ptr<ButtonAttachment> attachment;
    };
    struct Choice
    {
        juce::Rectangle<float> bounds;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<ComboAttachment> attachment;
    };
    struct Layer { juce::String name; juce::Image image; };
    void timerCallback() override;
    void choosePreset(bool save);
    void addKnob (const char* id, const char* label, float x, float y, float radius, int style);
    void addSwitch (const char* id, const char* label, juce::Rectangle<float> bounds);
    void addChoice (const char* id, const char* label, juce::StringArray values, juce::Rectangle<float> bounds);
    MicrophonicFuzzAudioProcessor& processor;
    std::unique_ptr<juce::LookAndFeel> style;
    std::vector<Layer> layers;
    std::vector<Knob> knobs;
    std::vector<Switch> switches;
    std::vector<Choice> choices;
    juce::TooltipWindow tooltip {this,500};
    juce::Image ledImage, vuFace, vuNeedle;
    bool showVU=false;
    juce::TextButton loadIR { "LOAD" };
    juce::TextButton irMenu {"..."};
    juce::TextButton previousIR { "<" }, nextIR { ">" }, savePreset { "Save" }, copyPreset { "Copy" }, undo { "Undo" }, redo { "Redo" }, settings { "..." }, roomA { "USER" }, roomB { "FACTORY" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String irStatus { "USER: load WAV" };
    std::array<float, 512> waveform {};
    std::array<float,128> irPreview {};
    std::array<float, 1024> fftData {};
    juce::dsp::FFT fft { 9 };
    float peak = 0.0f, vuRms=0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrophonicFuzzAudioProcessorEditor)
};
