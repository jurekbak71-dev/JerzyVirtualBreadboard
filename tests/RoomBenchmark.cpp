#include "RoomConvolution.h"
#include <juce_events/juce_events.h>
#include <chrono>
#include <iostream>
int main(){juce::ScopedJuceInitialiser_GUI initialiser;RoomConvolution room;room.prepare(48000);juce::AudioBuffer<float> impulse(1,48000*8);for(int i=0;i<impulse.getNumSamples();++i)impulse.setSample(0,i,static_cast<float>(std::exp(-i/80000.)*std::sin(i*.731)/80000));if(!room.stage(impulse,48000,true))return 1;double checksum=0;auto start=std::chrono::steady_clock::now();for(int i=0;i<48000;++i){auto result=room.readFrame();float input=std::sin(i*.1f)*.1f;room.pushFrame({input,input});checksum+=result[0]+result[1];}double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cout<<"8-second stereo partitioned IR,48kHz: "<<seconds<<"s wall/audio second ("<<seconds*100<<"% one core), checksum="<<checksum<<"\n";}
