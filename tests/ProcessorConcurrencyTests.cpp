#include "PluginProcessor.h"
#include <thread>
#include <atomic>
#include <iostream>
#include <cstdlib>
#include <chrono>
#define REQUIRE(c) do{if(!(c)){std::cerr<<"FAIL "<<#c<<" line "<<__LINE__<<"\n";std::abort();}}while(false)
static void validate(const juce::MemoryBlock& block){
 auto xml=juce::AudioProcessor::getXmlFromBinary(block.getData(),static_cast<int>(block.getSize()));REQUIRE(xml!=nullptr);
 auto state=juce::ValueTree::fromXml(*xml);auto data=state.getProperty("userIrData");auto* bytes=data.getBinaryData();REQUIRE(bytes!=nullptr);REQUIRE(bytes->getSize()%4==0);
 const double rate=static_cast<double>(state.getProperty("userIrRate"));REQUIRE(rate==44100||rate==48000||rate==96000);
 const double duration=static_cast<double>(bytes->getSize()/4)/rate;REQUIRE(std::abs(duration-.1)<=1/rate+1e-8);
}
int main(){
 juce::ScopedJuceInitialiser_GUI initialise;
 const auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("mf-concurrent-ir",".wav");
 {juce::WavAudioFormat format;std::unique_ptr<juce::OutputStream> stream=file.createOutputStream();REQUIRE(stream!=nullptr);auto writer=format.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(48000).withNumChannels(1).withBitsPerSample(24));REQUIRE(writer!=nullptr);juce::AudioBuffer<float> data(1,4800);data.clear();data.setSample(0,0,.5f);data.setSample(0,4799,.25f);REQUIRE(writer->writeFromAudioSampleBuffer(data,0,4800));}
 auto storage=std::make_unique<MicrophonicFuzzAudioProcessor>();auto& processor=*storage;processor.prepareToPlay(48000,1024);REQUIRE(processor.loadUserIR(file).wasOk());
 std::atomic<bool> start{false};std::atomic<int> finished{0},captures{0};
 std::thread loader([&]{while(!start.load())std::this_thread::yield();for(int i=0;i<120;++i)REQUIRE(processor.loadUserIR(file).wasOk());++finished;});
 std::thread hostRates([&]{while(!start.load())std::this_thread::yield();const double rates[]={44100,48000,96000};for(int i=0;i<90;++i)processor.prepareToPlay(rates[i%3],1024);++finished;});
 std::thread hostState([&]{while(!start.load())std::this_thread::yield();for(int i=0;i<240;++i){juce::MemoryBlock saved;processor.getStateInformation(saved);validate(saved);processor.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));++captures;}++finished;});
 start.store(true);
 // Service the actual processor and APVTS timers while the host workers contend.
 while(finished.load()!=3){juce::Timer::callPendingTimersSynchronously();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 loader.join();hostRates.join();hostState.join();REQUIRE(captures==240);
 juce::MemoryBlock finalState;processor.getStateInformation(finalState);validate(finalState);
 processor.prepareToPlay(48000,1024);juce::AudioBuffer<float> audio(2,1024);audio.clear();juce::MidiBuffer midi;processor.processBlock(audio,midi);REQUIRE(processor.hasUserIR());
 file.deleteFile();std::cout<<"PASS: concurrent120 IR loads,90 host rate changes,240 save/recalls with coherent embedded duration/rate and active timers; final IR activates\n";
}
