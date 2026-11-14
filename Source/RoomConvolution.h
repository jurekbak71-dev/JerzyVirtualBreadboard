#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

// One logical stereo room module. Prepared states are published off-thread;
// transient old/new states overlap for 50ms, then retire off the audio thread.
class RoomConvolution {
public:
 static constexpr int quantum=128;
 static constexpr double maximumSeconds=8.0;
 void prepare(double sampleRate) {
   rate=sampleRate;for(auto& s:slots){s.state.reset();s.owner.store(0);}pending.store(-1);active=-1;previous=-1;cursor=0;fade=1;fallback={};inputs={};loaded.store(false);kernelActive.store(false);
 }
 bool stage(const juce::AudioBuffer<float>& impulse,double sourceRate,bool user) {
   if(pending.load(std::memory_order_acquire)>=0)return false;
   int index=-1;for(int i=0;i<3;++i){int status=slots[static_cast<size_t>(i)].owner.load(std::memory_order_acquire);if(status==0||status==4){if(slots[static_cast<size_t>(i)].owner.compare_exchange_strong(status,1)){index=i;break;}}}
   if(index<0)return false;
   auto state=std::make_unique<State>();state->user=user;
   juce::AudioBuffer<float> owned;owned.makeCopyOf(impulse);
   state->convolution.loadImpulseResponse(std::move(owned),sourceRate,juce::dsp::Convolution::Stereo::yes,juce::dsp::Convolution::Trim::no,juce::dsp::Convolution::Normalise::no);
   state->convolution.prepare({rate,quantum,2});
   slots[static_cast<size_t>(index)].state=std::move(state);slots[static_cast<size_t>(index)].owner.store(2,std::memory_order_release);pending.store(index,std::memory_order_release);return true;
 }
 bool swapPending() const{return pending.load(std::memory_order_acquire)>=0;}
 bool hasKernel() const {return kernelActive.load(std::memory_order_acquire);}
 bool hasUserIR()const{return loaded.load(std::memory_order_acquire);}
 std::array<float,2> readFrame() {
   if(cursor==0&&previous<0){const int next=pending.load(std::memory_order_acquire);if(next>=0){previous=active;active=next;kernelActive.store(true,std::memory_order_release);slots[static_cast<size_t>(active)].owner.store(3,std::memory_order_release);loaded.store(slots[static_cast<size_t>(active)].state->user,std::memory_order_release);fade=0;pending.store(-1,std::memory_order_release);}}
   std::array<float,2> result{};
   for(int c=0;c<2;++c){float now=active>=0?slots[static_cast<size_t>(active)].state->output[static_cast<size_t>(c)][static_cast<size_t>(cursor)]:fallback[static_cast<size_t>(c)][static_cast<size_t>(cursor)];float old=previous>=0?slots[static_cast<size_t>(previous)].state->output[static_cast<size_t>(c)][static_cast<size_t>(cursor)]:fallback[static_cast<size_t>(c)][static_cast<size_t>(cursor)];result[static_cast<size_t>(c)]=old+(now-old)*fade;}
   fade=std::min(1.f,fade+static_cast<float>(1/(.05*rate)));return result;
 }
 void pushFrame(const std::array<float,2>& pickup) {
   for(int c=0;c<2;++c)inputs[static_cast<size_t>(c)][static_cast<size_t>(cursor)]=pickup[static_cast<size_t>(c)];
   if(++cursor==quantum){cursor=0;fallback=inputs;run(active);run(previous);if(previous>=0&&fade>=1){slots[static_cast<size_t>(previous)].owner.store(4,std::memory_order_release);previous=-1;}}
 }
private:
 struct State {juce::dsp::Convolution convolution{juce::dsp::Convolution::NonUniform{512}};std::array<std::array<float,quantum>,2> output{};bool user=false;};
 struct Slot {std::atomic<int> owner{0};std::unique_ptr<State> state;};
 std::array<Slot,3> slots;std::atomic<int> pending{-1};std::atomic<bool> loaded{false},kernelActive{false};
 int active=-1,previous=-1,cursor=0;float fade=1;double rate=44100;
 std::array<std::array<float,quantum>,2> inputs{},fallback{};
 void run(int index){if(index<0)return;auto& state=*slots[static_cast<size_t>(index)].state;state.output=inputs;float* pointers[]={state.output[0].data(),state.output[1].data()};juce::dsp::AudioBlock<float> block(pointers,2,quantum);juce::dsp::ProcessContextReplacing<float> context(block);state.convolution.process(context);}
};
