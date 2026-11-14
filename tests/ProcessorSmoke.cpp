#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI init;
        auto ownedProcessor=std::make_unique<MicrophonicFuzzAudioProcessor>();
        auto& processor=*ownedProcessor;
        require(processor.getName()=="Jerzy Microphonic Fuzz", "processor product identity");
        juce::MidiBuffer midi;
        for (double rate : {44100.0, 48000.0, 96000.0})
        {
            processor.prepareToPlay(rate, 2048);
            for (int size : {0, 1, 512, 2048})
            {
                juce::AudioBuffer<float> audio(2, size);
                audio.clear();
                processor.processBlock(audio, midi);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < size; ++i)
                        require(audio.getSample(ch, i) == 0.0f, "silence must stay silent");
            }
            processor.prepareToPlay(rate, 512);
            juce::AudioBuffer<float> audio(2, 512);
            audio.clear();
            audio.setSample(0, 0, 1.0f);
            processor.processBlock(audio, midi);
            require(audio.getMagnitude(0, 0, 512) > 0.001f, "impulse was not processed");
            require(audio.getMagnitude(1, 0, 512) == 0.0f, "stereo crosstalk");
            double energy=0;for(int ch=0;ch<2;++ch)for(int i=0;i<512;++i)energy+=std::pow(audio.getSample(ch,i),2);
            require(std::abs(processor.getOutputRms()-std::sqrt(energy/1024))<.00001,"RMS does not match actual output");
            for (int block = 0; block < 100; ++block)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                        audio.setSample(ch, i, 0.2f * std::sin(float((block * 512 + i) * 440.0 * 6.283185307179586 / rate)));
                processor.processBlock(audio, midi);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                        require(std::isfinite(audio.getSample(ch, i)) && std::abs(audio.getSample(ch, i)) <= 0.98001f, "tone exceeds enabled limiter ceiling");
            }
            processor.releaseResources();
        }
        auto* gain = processor.parameters.getParameter("gain");
        require(gain != nullptr, "gain ID changed");
        gain->setValueNotifyingHost(0.72f);
        juce::MemoryBlock state;
        processor.getStateInformation(state);
        auto ownedRestored=std::make_unique<MicrophonicFuzzAudioProcessor>();
        auto& restored=*ownedRestored;
        restored.setStateInformation(state.getData(), int(state.getSize()));
        for (auto id : {"gain", "feedback", "mix", "output"})
            require(std::abs(processor.parameters.getRawParameterValue(id)->load() - restored.parameters.getRawParameterValue(id)->load()) < 0.0001f, "state round trip failed");
        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
        require(editor != nullptr && editor->getWidth() > 0, "editor creation failed");
        for (auto size : {420, 720, 1200})
        {
            editor->setSize(size, size / 2);
            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            require(image.isValid(), "editor paint failed");
        }
        for(auto id:{"gain","bias","tone","gate","micDistance","micPosition","micMovement","micSensitivity","irMix","early","decay","feedback","resonance","instability","filter1Freq","filter1Res","filter2Freq","filter2Res","tremoloRate","tremoloDepth","flangerRate","flangerDepth","flangerFeedback","mix","output"})
        {
            auto* control=dynamic_cast<juce::Slider*>(editor->findChildWithID(id));auto* parameter=processor.parameters.getParameter(id);
            require(control!=nullptr&&parameter!=nullptr,"rack knob/parameter missing");const auto original=control->getValue();
            control->setValue(parameter->convertFrom0to1(.4f),juce::sendNotificationSync);
            require(std::abs(processor.parameters.getRawParameterValue(id)->load()-control->getValue())<.0001*juce::jmax(1.0,std::abs(control->getValue())),"knob is not attached");control->setValue(original,juce::sendNotificationSync);
        }
        for(auto id:{"bypass","limiter","tremoloOn","flangerOn"})
        {
            auto* control=dynamic_cast<juce::ToggleButton*>(editor->findChildWithID(id));require(control!=nullptr,"rack switch missing");const bool original=control->getToggleState();
            control->setToggleState(!original,juce::sendNotificationSync);require((processor.parameters.getRawParameterValue(id)->load()>.5f)==!original,"switch is not attached");control->setToggleState(original,juce::sendNotificationSync);
        }
        for(auto id:{"type","micMode","roomMode"})
        {
            auto* control=dynamic_cast<juce::ComboBox*>(editor->findChildWithID(id));require(control!=nullptr,"rack selector missing");const int original=control->getSelectedItemIndex();
            control->setSelectedItemIndex(control->getNumItems()-1,juce::sendNotificationSync);require(std::abs(processor.parameters.getParameter(id)->getValue()-1.0f)<.001f,"selector is not attached");control->setSelectedItemIndex(original,juce::sendNotificationSync);
        }
        // Capture the actual raster or fallback rotary control with three settings.
        auto* markerControl=dynamic_cast<juce::Slider*>(editor->findChildWithID("gain"));
        require(markerControl!=nullptr,"missing gain control for marker test");
        std::array<juce::Point<float>,3> redCenters{};
        juce::Colour outsideColor{};
        for(int state=0;state<3;++state){
            markerControl->setValue(state==0?1.0:state==1?15.5:30.0,juce::sendNotificationSync);
            const auto snapshot=markerControl->createComponentSnapshot(markerControl->getLocalBounds());
            require(snapshot.isValid(),"marker snapshot failed");
            int pixels=0;float sx=0,sy=0;
            for(int y=0;y<snapshot.getHeight();++y)for(int x=0;x<snapshot.getWidth();++x){
                const auto c=snapshot.getPixelAt(x,y);
                if(c.getRed()>160 && c.getRed()>c.getGreen()*1.4f && c.getRed()>c.getBlue()*1.4f){
                    ++pixels;sx+=float(x);sy+=float(y);
                }
            }
            require(pixels>=3,"red marker not rendered");
            redCenters[static_cast<size_t>(state)]={sx/pixels,sy/pixels};
            const auto corner=snapshot.getPixelAt(0,0);
            if(state==0)outsideColor=corner;
            else require(corner==outsideColor,"static corner changed outside moving marker");
        }
        require(redCenters[0].getDistanceFrom(redCenters[1])>1.0f &&
                redCenters[1].getDistanceFrom(redCenters[2])>1.0f,"red marker does not rotate");
        auto* gainControl=dynamic_cast<juce::Slider*>(editor->findChildWithID("gain"));
        require(gainControl != nullptr,"gain control missing from rack");
        gainControl->setValue(12.0,juce::sendNotificationSync);
        require(std::abs(processor.parameters.getRawParameterValue("gain")->load()-12.0f)<.001f,"GUI gain does not drive DSP parameter");
        const auto directory=juce::SystemStats::getEnvironmentVariable("MF_SCREENSHOT_DIR",{});
        if(directory.isNotEmpty())
        {
            juce::File folder(directory);folder.createDirectory();editor->setSize(1672,600);
            auto* source=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("roomMode"));source->setSelectedItemIndex(1,juce::sendNotificationSync);
            juce::AudioBuffer<float> signal(2,512);
            for(int block=0;block<8;++block)
            {
                for(int ch=0;ch<2;++ch)for(int i=0;i<512;++i)signal.setSample(ch,i,.15f*std::sin(float((block*512+i)*440*6.283185307/processor.getProcessingSampleRate())));
                processor.processBlock(signal,midi);juce::Thread::sleep(50);juce::Timer::callPendingTimersSynchronously();
            }
            auto saveSnapshot=[&](const char* name)
            {
                juce::MemoryOutputStream imageBytes;
                require(juce::PNGImageFormat().writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds()),imageBytes),"snapshot encoding failed");
                require(folder.getChildFile(name).replaceWithData(imageBytes.getData(),imageBytes.getDataSize()),"snapshot save failed");
            };
            saveSnapshot("rack-gui.png");
            auto* rack=dynamic_cast<MicrophonicFuzzAudioProcessorEditor*>(editor.get());require(rack!=nullptr,"rack editor type");
            rack->setAnalyzerMode(true);saveSnapshot("rack-vu.png");
        }
        std::cout << "PASS: silence, impulse, stereo isolation, tone, 3 rates, 4 block sizes, state, all25knobs/4switches/3selectors attached, editor paint/resize and RMS\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
