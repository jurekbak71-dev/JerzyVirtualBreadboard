#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

MicrophonicFuzzAudioProcessor::MicrophonicFuzzAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, &undoManager, "Parameters", makeLayout())
{ startTimer(50); }

juce::AudioProcessorValueTreeState::ParameterLayout MicrophonicFuzzAudioProcessor::makeLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&p] (const char* id, const char* name, float min, float max, float def)
    {
        juce::NormalisableRange<float> range(min,max);
        if(min==20.f && max==20000.f)range.setSkewForCentre(1000.f);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def));
    };
    add ("gain", "Fuzz Gain", 1.0f, 30.0f, 6.0f);
    add ("feedback", "Feedback", 0.0f, 0.85f, 0.2f);
    add ("mix", "Mix", 0.0f, 1.0f, 1.0f);
    add ("output", "Output", 0.0f, 2.0f, 0.8f);
    add ("bias", "Bias", -1, 1, 0);
    add ("tone", "Tone", 0, 1, .5f);
    add ("gate", "Gate", 0, 1, 0);
    add ("type", "Fuzz Type", 0, 3, 0);
    add ("micDistance", "Mic Distance", 0, 1, .3f);
    add ("micPosition", "Mic Position", 0, 1, .5f);
    add ("micMovement", "Mic Movement", 0, 1, 0);
    add ("micSensitivity", "Mic Sensitivity", 0, 2, 1);
    add ("micMode", "Mic Mode", 0, 1, 0);
    add ("resonance", "Resonance", 0, 1, .3f);
    add ("instability", "Instability", 0, 1, 0);
    add ("filter1Freq", "Low Pass", 20, 20000, 5000);
    add ("filter1Res", "Low Pass Emphasis", 0, 1, 0);
    add ("filter2Freq", "High Pass", 20, 20000, 80);
    add ("filter2Res", "High Pass Emphasis", 0, 1, 0);
    add ("tremoloRate", "Tremolo Rate", .1f, 20, 4);
    add ("tremoloDepth", "Tremolo Depth", 0, 1, 0);
    add ("tremoloOn", "Tremolo On", 0, 1, 0);
    add ("flangerRate", "Flanger Rate", .05f, 5, .3f);
    add ("flangerDepth", "Flanger Depth", 0, 1, 0);
    add ("flangerFeedback", "Flanger Feedback", 0, 1, 0);
    add ("flangerOn", "Flanger On", 0, 1, 0);
    add ("limiter", "Limiter", 0, 1, 1);
    add ("bypass", "Bypass", 0, 1, 0);
    add ("roomMode", "IR Source USER/FACTORY", 0, 1, 0);
    add ("factoryIR", "Factory Room Selection", 0, 4, 0);
    add ("irMix", "IR Mix", 0, 1, .5f);
    add ("early", "Early Reflections", 0, 1, .3f);
    add ("decay", "Reflection Decay", 0, 1, .4f);
    return { p.begin(), p.end() };
}

bool MicrophonicFuzzAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
         || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
void MicrophonicFuzzAudioProcessor::prepareToPlay (double rate, int)
{
    rate=std::clamp(std::isfinite(rate)?rate:44100.0,8000.0,384000.0);
    { std::lock_guard<std::mutex> guard(irLoadingMutex);processingRate.store(rate);engine.prepare(rate);engine.clearIR();room.prepare(rate); }
    restoreEmbeddedIR();
    outputPeak.store(0);outputRms.store(0);for(auto& value:channelPeaks)value.store(0);
}
void MicrophonicFuzzAudioProcessor::releaseResources() {}
void MicrophonicFuzzAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    FuzzEngine::Parameters p;
#define READ(x) p.x=parameters.getRawParameterValue(#x)->load(std::memory_order_relaxed)
    READ(gain); READ(feedback); READ(mix); READ(output); READ(bias); READ(tone); READ(gate); READ(type);
    READ(micDistance); READ(micPosition); READ(micMovement); READ(micSensitivity); READ(micMode);
    READ(resonance); READ(instability); READ(filter1Freq); READ(filter1Res); READ(filter2Freq); READ(filter2Res);
    READ(tremoloRate); READ(tremoloDepth); READ(tremoloOn); READ(flangerRate); READ(flangerDepth); READ(flangerFeedback); READ(flangerOn);
    READ(limiter); READ(bypass); READ(roomMode); READ(irMix); READ(early); READ(decay);
#undef READ
    if(buffer.getNumChannels()<1) return;
    engine.beginBlock();
    float peak=0;std::array<float,2> stereoPeaks{};double meterEnergy=0;
    auto pos=waveformPosition.load(std::memory_order_relaxed);
    for(int i=0;i<buffer.getNumSamples();++i) {
        float l=buffer.getSample(0,i),r=buffer.getNumChannels()>1?buffer.getSample(1,i):l;
        const auto reflected=room.readFrame();std::array<float,2> pickup{};
        engine.process(l,r,p,buffer.getNumChannels()>1,&reflected,&pickup);room.pushFrame(pickup);
        buffer.setSample(0,i,l);if(buffer.getNumChannels()>1)buffer.setSample(1,i,r);
        meterEnergy+=(double(l)*l+double(r)*r)*.5;
        peak=std::max(peak,std::max(std::abs(l),std::abs(r)));
        stereoPeaks[0]=std::max(stereoPeaks[0],std::abs(l));stereoPeaks[1]=std::max(stereoPeaks[1],std::abs(r));
        waveform[(pos++)%waveform.size()].store((l+r)*.5f,std::memory_order_relaxed);
    }
    for(int ch=2;ch<buffer.getNumChannels();++ch) buffer.clear(ch,0,buffer.getNumSamples());
    waveformPosition.store(pos,std::memory_order_release);
    outputRms.store(buffer.getNumSamples()>0?float(std::sqrt(meterEnergy/buffer.getNumSamples())):0.0f,std::memory_order_relaxed);
    outputPeak.store(std::max(peak,outputPeak.load()* .9f),std::memory_order_relaxed);
    for(size_t channel=0;channel<channelPeaks.size();++channel)channelPeaks[channel].store(std::max(stereoPeaks[channel],channelPeaks[channel].load(std::memory_order_relaxed)*.9f),std::memory_order_relaxed);
    micVisualOffset.store(engine.getMicVisualOffset(),std::memory_order_relaxed);
    feedbackActivity.store(engine.getFeedbackActivity(),std::memory_order_relaxed);
}
void MicrophonicFuzzAudioProcessor::copyRoomIRPreview(float* destination,int size) const
{
    if(destination==nullptr || size<=0)return;
    for(int i=0;i<size;++i)destination[i]=roomIRPreview[static_cast<size_t>(std::min(127,static_cast<int>(static_cast<long long>(i)*128/size)))].load(std::memory_order_relaxed);
}
void MicrophonicFuzzAudioProcessor::updateRoomIRPreview(const juce::AudioBuffer<float>* impulse,double rate)
{
    std::array<float,128> preview{};float maximum=0;
    if(impulse!=nullptr){
        const int length=impulse->getNumSamples();
        for(int i=0;i<length;++i){const size_t bin=static_cast<size_t>(std::min(127,static_cast<int>(static_cast<long long>(i)*128/length)));const float sample=impulse->getSample(0,i);if(std::abs(sample)>std::abs(preview[bin]))preview[bin]=sample;maximum=std::max(maximum,std::abs(sample));}
        if(maximum>0)for(auto& sample:preview)sample/=maximum;
        roomIRDuration.store(length/rate,std::memory_order_relaxed);
    }else roomIRDuration.store(0,std::memory_order_relaxed);
    for(size_t i=0;i<preview.size();++i)roomIRPreview[i].store(preview[i],std::memory_order_relaxed);
}
int MicrophonicFuzzAudioProcessor::copyWaveform(float* destination,int maxSamples) const
{
    const int n=std::clamp(maxSamples,0,512);const unsigned pos=waveformPosition.load(std::memory_order_acquire);
    for(int i=0;i<n;++i)destination[i]=waveform[(pos+512-static_cast<unsigned>(n)+static_cast<unsigned>(i))%512].load(std::memory_order_relaxed);
    return n;
}
juce::Result MicrophonicFuzzAudioProcessor::loadUserIR(const juce::File& file)
{
    std::unique_lock<std::mutex> guard(irLoadingMutex);
    juce::AudioFormatManager formats;formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if(!reader)return juce::Result::fail("Cannot read WAV/AIFF IR.");
    if(!std::isfinite(reader->sampleRate) || reader->numChannels<1 || reader->sampleRate<8000 || reader->sampleRate>384000 || reader->lengthInSamples<1 || reader->lengthInSamples>3072000 || reader->numChannels>2)
        return juce::Result::fail("IR must be mono/stereo, 8–384 kHz, and at most 3,072,000 source frames.");
    const double loadRate=processingRate.load();
    const double ratio=reader->sampleRate/loadRate;
    const auto count=static_cast<int>(std::ceil(static_cast<double>(reader->lengthInSamples)/ratio));
    if(count<1 || count>static_cast<int>(loadRate*RoomConvolution::maximumSeconds))return juce::Result::fail("IR exceeds the explicit 8-second feedback convolution limit. No truncation performed.");
    juce::AudioBuffer<float> source(static_cast<int>(reader->numChannels),static_cast<int>(reader->lengthInSamples)+1);
    source.clear();if(!reader->read(&source,0,static_cast<int>(reader->lengthInSamples),0,true,true)) return juce::Result::fail("IR decoding failed.");
    std::vector<float> mono(static_cast<size_t>(count));
    for(int i=0;i<count;++i) {const double offset=i*ratio;const int j=static_cast<int>(offset);const float frac=static_cast<float>(offset-j);
        for(int ch=0;ch<source.getNumChannels();++ch)mono[static_cast<size_t>(i)]+=(source.getSample(ch,j)*(1-frac)+source.getSample(ch,std::min(j+1,source.getNumSamples()-1))*frac)/static_cast<float>(source.getNumChannels());}
    double sum=0;for(float value:mono){if(!std::isfinite(value))return juce::Result::fail("IR has invalid samples.");sum+=std::abs(value);}
    if(sum<1e-10)return juce::Result::fail("IR is silent.");
    juce::MemoryBlock bytes;
    { juce::MemoryOutputStream stream(bytes,false);
      for(int i=0;i<count;++i) stream.writeFloat(mono[static_cast<size_t>(i)]); }
    parameters.state.setProperty("userIrData",juce::var(bytes.getData(),bytes.getSize()),nullptr);
    parameters.state.setProperty("userIrRate",loadRate,nullptr);
    parameters.state.setProperty("userIrPath",file.getFullPathName(),nullptr);
    guard.unlock();handleAsyncUpdate();return juce::Result::ok();
}

juce::Result MicrophonicFuzzAudioProcessor::restoreEmbeddedIR()
{
    std::lock_guard<std::mutex> guard(irLoadingMutex);
    if(room.swapPending()) return juce::Result::fail("IR swap pending; restore will retry.");
    const auto data=parameters.state.getProperty("userIrData");const auto* bytes=data.getBinaryData();
    const double rate=processingRate.load();
    const int mode=parameters.getRawParameterValue("roomMode")->load()>=.5f?1:0;
    const int factoryIndex=juce::jlimit(0,JerzyFactoryIR::count-1,juce::roundToInt(parameters.getRawParameterValue("factoryIR")->load()));
    const float decay=std::clamp(parameters.getRawParameterValue("decay")->load(),0.f,1.f);
    const bool hasUser=bytes!=nullptr && bytes->getSize()>0;
    if(!hasUser&&mode==0&&!room.hasKernel()){updateRoomIRPreview(nullptr,rate);roomModeApplied=mode;factoryIRApplied=factoryIndex;roomDecayApplied=decay;return juce::Result::ok();}
    juce::AudioBuffer<float> impulse(1,1);impulse.clear();impulse.setSample(0,0,1);
    if(hasUser){
        const double sourceRate=static_cast<double>(parameters.state.getProperty("userIrRate",44100.0));
        if(!std::isfinite(sourceRate)||sourceRate<8000||sourceRate>384000||bytes->getSize()%4!=0||bytes->getSize()>static_cast<size_t>(384000*RoomConvolution::maximumSeconds*4))return juce::Result::fail("Invalid embedded IR data.");
        const int originalCount=static_cast<int>(bytes->getSize()/4);const double ratio=sourceRate/rate;
        const int count=static_cast<int>(std::ceil(originalCount/ratio));
        if(count<1||count>static_cast<int>(rate*RoomConvolution::maximumSeconds))return juce::Result::fail("Embedded IR exceeds 8-second limit; original retained.");
        std::vector<float> source(static_cast<size_t>(originalCount));juce::MemoryInputStream input(*bytes,false);
        for(auto& value:source){value=input.readFloat();if(!std::isfinite(value))return juce::Result::fail("Invalid embedded sample.");}
        impulse.setSize(1,count);
        for(int i=0;i<count;++i){const double offset=i*ratio;const int j=std::min(static_cast<int>(offset),originalCount-1);const float fraction=static_cast<float>(offset-j);impulse.setSample(0,i,source[static_cast<size_t>(j)]*(1-fraction)+source[static_cast<size_t>(std::min(j+1,originalCount-1))]*fraction);}
    }
    if(hasUser){double userNorm=0;for(int i=0;i<impulse.getNumSamples();++i)userNorm+=std::abs(impulse.getSample(0,i));if(userNorm>1)impulse.applyGain(static_cast<float>(1/userNorm));}
    if(mode==1){
        auto factory=JerzyFactoryIR::makeSyntheticFactoryRoom(factoryIndex);const double ratio=48000/rate;const int count=static_cast<int>(std::ceil(factory.getNumSamples()/ratio));
        const int oldCount=hasUser?impulse.getNumSamples():0;
        impulse.setSize(1,std::max(oldCount,count),true,true);if(!hasUser)impulse.clear();
        for(int i=0;i<count;++i){const double offset=i*ratio;const int j=std::min(static_cast<int>(offset),factory.getNumSamples()-1);const float fraction=static_cast<float>(offset-j);const float value=factory.getSample(0,j)*(1-fraction)+factory.getSample(0,std::min(j+1,factory.getNumSamples()-1))*fraction;impulse.addSample(0,i,value);}
    }
    // Additional physical T60 envelope; source IR decay remains part of the result.
    const double t60=.1+9.9*decay;
    for(int i=0;i<impulse.getNumSamples();++i)impulse.setSample(0,i,impulse.getSample(0,i)*static_cast<float>(std::exp(-std::log(1000.)*i/(rate*t60))));
    double norm=0;for(int i=0;i<impulse.getNumSamples();++i)norm+=std::abs(impulse.getSample(0,i));
    if(norm<1e-10)return juce::Result::fail("IR is silent.");impulse.applyGain(static_cast<float>(1/std::max(1.,norm)));
    if(!room.stage(impulse,rate,hasUser))return juce::Result::fail("IR transition in progress; restore will retry.");
    updateRoomIRPreview(hasUser||mode==1?&impulse:nullptr,rate);
    roomModeApplied=mode;factoryIRApplied=factoryIndex;roomDecayApplied=decay;return juce::Result::ok();
}
void MicrophonicFuzzAudioProcessor::handleAsyncUpdate()
{
    // Clear before reading locked state: a newer concurrent request cannot be
    // overwritten by an older successful restore after it releases the lock.
    retryIR.store(false);
    if(restoreEmbeddedIR().failed()) retryIR.store(true);
}
void MicrophonicFuzzAudioProcessor::timerCallback()
{
    const int mode=parameters.getRawParameterValue("roomMode")->load()>=.5f?1:0;
    const int factoryIndex=juce::jlimit(0,JerzyFactoryIR::count-1,
        juce::roundToInt(parameters.getRawParameterValue("factoryIR")->load()));
    if(retryIR.load()||mode!=roomModeApplied.load()
       ||(mode==1 && factoryIndex!=factoryIRApplied.load())
       ||std::abs(parameters.getRawParameterValue("decay")->load()-roomDecayApplied.load())>.0001f)
        handleAsyncUpdate();
}

void MicrophonicFuzzAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    std::lock_guard<std::mutex> guard(irLoadingMutex);
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void MicrophonicFuzzAudioProcessor::setStateInformation (const void* data, int size)
{
    const auto xml=getXmlFromBinary(data,size);
    if(xml==nullptr || !xml->hasTagName("Parameters")) return;
    const auto restored=juce::ValueTree::fromXml(*xml);
    {
        std::lock_guard<std::mutex> guard(irLoadingMutex);
        parameters.replaceState(restored);
    }
    // Posting is deliberately outside the state/IR lock; processing never acquires it.
    triggerAsyncUpdate();
}

juce::AudioProcessorEditor* MicrophonicFuzzAudioProcessor::createEditor()
{
    return new MicrophonicFuzzAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicrophonicFuzzAudioProcessor();
}
