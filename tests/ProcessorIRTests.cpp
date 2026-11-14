#include "PluginProcessor.h"
#include <iostream>
#include <cstdlib>
#include <set>
#include <cstdint>
#define REQUIRE(condition) do {if(!(condition)){std::cerr<<"FAIL "<<#condition<<" line "<<__LINE__<<"\n";std::abort();}}while(false)
static void process(MicrophonicFuzzAudioProcessor& p){juce::AudioBuffer<float> b(2,1024);b.clear();juce::MidiBuffer midi;p.processBlock(b,midi);}
static void writeIR(const juce::File& file,int count){juce::WavAudioFormat format;std::unique_ptr<juce::OutputStream> stream=file.createOutputStream();REQUIRE(stream!=nullptr);auto writer=format.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(48000).withNumChannels(1).withBitsPerSample(24));REQUIRE(writer!=nullptr);juce::AudioBuffer<float> data(1,count);data.clear();data.setSample(0,0,.75f);data.setSample(0,count-1,.25f);REQUIRE(writer->writeFromAudioSampleBuffer(data,0,count));}
int main(){
 juce::ScopedJuceInitialiser_GUI initialise;
 const auto path=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("microphonic-ir-regression",".wav");
 writeIR(path,128);
 auto originalStorage=std::make_unique<MicrophonicFuzzAudioProcessor>();auto& original=*originalStorage;original.prepareToPlay(48000,1024);
 std::array<float,128> preview{};original.copyRoomIRPreview(preview.data(),128);for(float sample:preview)REQUIRE(sample==0);REQUIRE(original.getRoomIRDuration()==0);
 {juce::AudioBuffer<float> leftImpulse(2,1024);leftImpulse.clear();leftImpulse.setSample(0,0,1);juce::MidiBuffer midi;original.processBlock(leftImpulse,midi);REQUIRE(original.getOutputPeak(0)>.001f);REQUIRE(original.getOutputPeak(1)==0);}
 original.prepareToPlay(48000,1024);REQUIRE(original.loadUserIR(path).wasOk());process(original);REQUIRE(original.hasUserIR());
 original.copyRoomIRPreview(preview.data(),128);REQUIRE(std::abs(preview.front()-1)<.00001f);REQUIRE(std::abs(preview.back()-(1.f/3)*std::exp(-std::log(1000.)*127/(48000*(.1+9.9*.4))))<.00001f);for(int i=1;i<127;++i)REQUIRE(preview[static_cast<size_t>(i)]==0);REQUIRE(std::abs(original.getRoomIRDuration()-128./48000)<1e-10);
 auto* output=original.parameters.getParameter("output");output->setValueNotifyingHost(output->convertTo0to1(.37f));
 juce::MemoryBlock saved;original.getStateInformation(saved);REQUIRE(saved.getSize()>0);
 REQUIRE(original.parameters.state.getProperty("userIrData").getBinaryData()->getSize()==128*4);
 original.undoManager.beginNewTransaction("Output regression");output->setValueNotifyingHost(output->convertTo0to1(.6f));original.parameters.copyState();REQUIRE(original.undoManager.canUndo());REQUIRE(original.undoManager.undo());REQUIRE(std::abs(original.parameters.getRawParameterValue("output")->load()-.37f)<.0001f);REQUIRE(original.undoManager.redo());REQUIRE(std::abs(original.parameters.getRawParameterValue("output")->load()-.6f)<.0001f);
 path.deleteFile(); // Preset restore must not depend on source path availability.
 auto recalledStorage=std::make_unique<MicrophonicFuzzAudioProcessor>();auto& recalled=*recalledStorage;recalled.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));recalled.prepareToPlay(48000,1024);process(recalled);REQUIRE(recalled.hasUserIR());REQUIRE(std::abs(recalled.parameters.getRawParameterValue("output")->load()-.37f)<.0001f);
 recalled.prepareToPlay(96000,1024);process(recalled);REQUIRE(recalled.hasUserIR());
 // Long IR survives sample-rate changes without the obsolete short-FIR cap.
 writeIR(path,48000);auto longStorage=std::make_unique<MicrophonicFuzzAudioProcessor>();auto& longIR=*longStorage;longIR.prepareToPlay(48000,1024);REQUIRE(longIR.loadUserIR(path).wasOk());process(longIR);REQUIRE(longIR.hasUserIR());longIR.prepareToPlay(96000,1024);process(longIR);REQUIRE(longIR.hasUserIR());REQUIRE(longIR.parameters.state.getProperty("userIrData").getBinaryData()->getSize()==48000*4);longIR.prepareToPlay(48000,1024);process(longIR);REQUIRE(longIR.hasUserIR());path.deleteFile();
 writeIR(path,48000*8+1);REQUIRE(longIR.loadUserIR(path).failed());path.deleteFile();
 // FACTORY selects one of five synthetic room models through the same convolution module.
 auto factoryStorage=std::make_unique<MicrophonicFuzzAudioProcessor>();auto userOnlyStorage=std::make_unique<MicrophonicFuzzAudioProcessor>();
 auto& factoryProcessor=*factoryStorage;auto& userOnlyProcessor=*userOnlyStorage;
 factoryProcessor.parameters.getParameter("roomMode")->setValueNotifyingHost(1);
 factoryProcessor.prepareToPlay(48000,512);userOnlyProcessor.prepareToPlay(48000,512);REQUIRE(factoryProcessor.hasFactoryIR());
 constexpr int expectedFrames[]={21600,43200,69600,110400,177600};
 std::set<std::uint64_t> signatures;
 auto* factoryChoice=factoryProcessor.parameters.getParameter("factoryIR");REQUIRE(factoryChoice!=nullptr);
 for(int selected=0;selected<JerzyFactoryIR::count;++selected){
     factoryChoice->setValueNotifyingHost(factoryChoice->convertTo0to1(float(selected)));
     factoryProcessor.prepareToPlay(48000,512);
     factoryProcessor.copyRoomIRPreview(preview.data(),128);
     float factoryPreviewPeak=0;
     std::uint64_t signature=1469598103934665603ULL;
     for(float sample:preview){
         factoryPreviewPeak=std::max(factoryPreviewPeak,std::abs(sample));
         signature^=static_cast<std::uint64_t>(std::abs(juce::roundToInt(sample*100000.0f)));
         signature*=1099511628211ULL;
     }
     REQUIRE(factoryPreviewPeak>.99f);
     REQUIRE(std::abs(factoryProcessor.getRoomIRDuration()-expectedFrames[selected]/48000.)<1e-10);
     signatures.insert(signature);
 }
 REQUIRE(signatures.size()==5);
 double modeDifference=0;juce::MidiBuffer midi;
 for(int block=0;block<100;++block){juce::AudioBuffer<float> a(2,512),b(2,512);for(int c=0;c<2;++c)for(int i=0;i<512;++i){float x=.15f*std::sin((block*512+i)*.17f);a.setSample(c,i,x);b.setSample(c,i,x);}factoryProcessor.processBlock(a,midi);userOnlyProcessor.processBlock(b,midi);if(block>10)for(int i=0;i<512;++i)modeDifference+=std::abs(a.getSample(0,i)-b.getSample(0,i));}
 REQUIRE(modeDifference>.001);
 juce::MemoryBlock factoryPreset;factoryProcessor.getStateInformation(factoryPreset);
 auto restoredFactoryStorage=std::make_unique<MicrophonicFuzzAudioProcessor>();auto& restoredFactory=*restoredFactoryStorage;
 restoredFactory.setStateInformation(factoryPreset.getData(),int(factoryPreset.getSize()));
 restoredFactory.prepareToPlay(48000,512);
 REQUIRE(std::abs(restoredFactory.parameters.getRawParameterValue("factoryIR")->load()-4.0f)<.001f);
 REQUIRE(restoredFactory.parameters.getRawParameterValue("roomMode")->load()>.5f);
 REQUIRE(std::abs(restoredFactory.getRoomIRDuration()-177600./48000)<1e-10);
 // Decay adds a0.1..10second T60 envelope without modifying portable raw IR bytes.
 auto rawBefore=longIR.parameters.state.getProperty("userIrData");const juce::MemoryBlock rawCopy=*rawBefore.getBinaryData();
 longIR.parameters.getParameter("decay")->setValueNotifyingHost(0);longIR.prepareToPlay(48000,1024);longIR.copyRoomIRPreview(preview.data(),128);const float shortTail=preview.back();
 longIR.parameters.getParameter("decay")->setValueNotifyingHost(1);longIR.prepareToPlay(48000,1024);longIR.copyRoomIRPreview(preview.data(),128);const float longTail=preview.back();
 REQUIRE(longTail>shortTail+.01f);REQUIRE(*longIR.parameters.state.getProperty("userIrData").getBinaryData()==rawCopy);
 for(const char* id:{"filter1Freq","filter2Freq"}){auto* frequency=longIR.parameters.getParameter(id);REQUIRE(std::abs(frequency->convertFrom0to1(0)-20)<.001f);REQUIRE(std::abs(frequency->convertFrom0to1(1)-20000)<.01f);REQUIRE(std::abs(frequency->convertFrom0to1(.5f)-1000)<.01f);}
 REQUIRE(longIR.getTailLengthSeconds()==60);
 std::cout<<"PASS: embedded IR preset recall without source file, parameter recall, sample-rate resampling, long-IR preservation, render-tail hint\n";
}
