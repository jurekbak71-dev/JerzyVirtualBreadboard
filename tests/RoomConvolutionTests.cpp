#include "RoomConvolution.h"
#include <juce_events/juce_events.h>
#include "FuzzEngine.h"
#include <iostream>
#include <cstdlib>
#define REQUIRE(c) do{if(!(c)){std::cerr<<"FAIL "<<#c<<" line "<<__LINE__<<"\n";std::abort();}}while(false)
int main(){
 juce::ScopedJuceInitialiser_GUI initialise;
 RoomConvolution room;room.prepare(48000);juce::AudioBuffer<float> ir(1,48000);ir.clear();ir.setSample(0,0,.5f);ir.setSample(0,123,.2f);ir.setSample(0,47000,.1f);REQUIRE(room.stage(ir,48000,true));
 // Settle the50ms transition with silence before measuring the FIR impulse.
 for(int i=0;i<4096;++i){room.readFrame();room.pushFrame({0,0});}
 double error=0;
 for(int i=0;i<49000;++i){const auto out=room.readFrame();float expected=0;if(i==128)expected=.5f;if(i==128+123)expected=.2f;if(i==128+47000)expected=.1f;error+=std::abs(out[0]-expected)+std::abs(out[1]-expected);room.pushFrame({i==0?1.f:0.f,i==0?1.f:0.f});}
 REQUIRE(error<.001);
 auto compare=[](float feedback){RoomConvolution first,second;first.prepare(48000);second.prepare(48000);juce::AudioBuffer<float> impulse(1,4000);impulse.clear();impulse.setSample(0,3000,1);REQUIRE(second.stage(impulse,48000,true));FuzzEngine a,b;a.prepare(48000);b.prepare(48000);FuzzEngine::Parameters p;p.feedback=feedback;p.irMix=1;p.early=0;double difference=0;for(int i=0;i<50000;++i){float x=i<10000?0.f:std::sin(i*.13f)*.15f;float l=x,r=x,bl=x,br=x;auto ar=first.readFrame(),brm=second.readFrame();std::array<float,2> ap{},bp{};a.process(l,r,p,true,&ar,&ap);b.process(bl,br,p,true,&brm,&bp);first.pushFrame(ap);second.pushFrame(bp);if(i>15000)difference+=std::abs(l-bl);}return difference;};
 REQUIRE(compare(0)<1e-5);REQUIRE(compare(.8f)>.01);
 std::cout<<"PASS: one-second partitioned IR impulse equals direct FIR with128sample feedback pipeline delay; longIR affects only feedback\n";
}
