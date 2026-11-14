#include <juce_events/juce_events.h>
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
using namespace Steinberg;
static void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
class Host final : public Vst::IHostApplication
{
public:
    tresult PLUGIN_API queryInterface(const TUID id,void** out) override
    {
        if(std::memcmp(id,Vst::IHostApplication_iid,16)==0||std::memcmp(id,FUnknown_iid,16)==0){*out=this;return kResultOk;}*out=nullptr;return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override{return 1;}
    uint32 PLUGIN_API release() override{return 1;}
    tresult PLUGIN_API getName(Vst::String128 name) override{const char* text="Microphonic Fuzz binary smoke host";for(int i=0;i<128;++i){name[i]=char16_t(*text);if(*text==0)break;++text;}return kResultOk;}
    tresult PLUGIN_API createInstance(TUID,TUID,void** out) override{*out=nullptr;return kNotImplemented;}
};
int main(int argc,char** argv)
{
 try{
    check(argc==2,"expected VST3 binary path");juce::ScopedJuceInitialiser_GUI init;
#if defined(_WIN32)
    auto handle=LoadLibraryW(juce::String::fromUTF8(argv[1]).toWideCharPointer());
    check(handle!=nullptr,"LoadLibrary VST3 failed");auto symbol=[handle](const char* name){return GetProcAddress(handle,name);};
#else
    auto handle=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);check(handle!=nullptr,dlerror());auto symbol=[handle](const char* name){return dlsym(handle,name);};
    auto entry=reinterpret_cast<bool(*)(void*)>(symbol("ModuleEntry"));check(entry!=nullptr&&entry(handle),"VST3 ModuleEntry failed");
#endif
    auto factoryEntry=reinterpret_cast<IPluginFactory*(*)()>(symbol("GetPluginFactory"));check(factoryEntry!=nullptr,"missing GetPluginFactory");auto* factory=factoryEntry();check(factory!=nullptr&&factory->countClasses()>0,"empty factory");
    Vst::IComponent* component=nullptr;
    for(int i=0;i<factory->countClasses();++i){PClassInfo info{};if(factory->getClassInfo(i,&info)==kResultOk&&std::strcmp(info.category,kVstAudioEffectClass)==0){check(factory->createInstance(info.cid,Vst::IComponent_iid,reinterpret_cast<void**>(&component))==kResultOk,"create component");break;}}
    check(component!=nullptr,"no audio effect class");Host host;check(component->initialize(&host)==kResultOk,"initialize component");
    Vst::IAudioProcessor* audio=nullptr;check(component->queryInterface(Vst::IAudioProcessor_iid,reinterpret_cast<void**>(&audio))==kResultOk,"no audio processor");
    Vst::SpeakerArrangement inputs=Vst::SpeakerArr::kStereo,outputs=inputs;check(audio->setBusArrangements(&inputs,1,&outputs,1)==kResultOk,"stereo arrangement");
    Vst::ProcessSetup setup{};setup.processMode=Vst::kRealtime;setup.symbolicSampleSize=Vst::kSample32;setup.maxSamplesPerBlock=512;setup.sampleRate=48000;
    check(audio->setupProcessing(setup)==kResultOk,"setupProcessing");check(component->activateBus(Vst::kAudio,Vst::kInput,0,true)==kResultOk,"input bus");check(component->activateBus(Vst::kAudio,Vst::kOutput,0,true)==kResultOk,"output bus");check(component->setActive(true)==kResultOk,"activate plugin");check(audio->setProcessing(true)==kResultOk,"start processing");
    float in[2][512]{},out[2][512]{};float* ip[]={in[0],in[1]};float* op[]={out[0],out[1]};Vst::AudioBusBuffers input{},output{};input.numChannels=2;input.channelBuffers32=ip;output.numChannels=2;output.channelBuffers32=op;
    Vst::ProcessData data{};data.processMode=Vst::kRealtime;data.symbolicSampleSize=Vst::kSample32;data.numSamples=512;data.numInputs=1;data.numOutputs=1;data.inputs=&input;data.outputs=&output;
    double energy=0;
    for(int block=0;block<32;++block){for(int i=0;i<512;++i)in[0][i]=in[1][i]=.2f*std::sin(float((block*512+i)*440*6.283185307/48000));check(audio->process(data)==kResultOk,"process returned error");for(auto& channel:out)for(float sample:channel){check(std::isfinite(sample)&&std::abs(sample)<=.981f,"invalid output");energy+=sample*sample;}}
    check(energy>.001,"VST3 produced no audio");audio->setProcessing(false);component->setActive(false);component->terminate();audio->release();component->release();factory->release();
#if defined(_WIN32)
    FreeLibrary(handle);
#else
    auto exitModule=reinterpret_cast<bool(*)()>(symbol("ModuleExit"));check(exitModule!=nullptr&&exitModule(),"VST3 ModuleExit failed");dlclose(handle);
#endif
    std::cout<<"PASS: actual VST3 factory, create/init, stereo buses,48k/512,16384frames tone, finite bounded audio, terminate/unload\n";return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL: "<<(error.what()?error.what():"module load error")<<'\n';return 1;}
}
