#ifdef _WIN32
#include "live_audio_win.hpp"
#include "realtime_circuit.hpp"
#include <miniaudio.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace vbal {
namespace {
constexpr ma_backend kBackends[]={ma_backend_wasapi};
float dbToGain(float db){return std::pow(10.f,std::clamp(db,-60.f,36.f)/20.f);}
}
struct LiveAudioEngine::Impl {
 ma_context context{};
 bool contextInitialized=false;
 ma_device device{};
 bool initialized=false;
 ma_decoder decoder{};
 bool decoderInitialized=false;
 std::vector<float> scratch=std::vector<float>(8192*2);
};
LiveAudioEngine::LiveAudioEngine():impl_(std::make_unique<Impl>()){}
LiveAudioEngine::~LiveAudioEngine(){stop();}
AudioDeviceLists LiveAudioEngine::enumerateDevices(){
 AudioDeviceLists result;
 ma_context context{};
 if(ma_context_init(kBackends,1,nullptr,&context)!=MA_SUCCESS)return result;
 ma_device_info *playback=nullptr,*capture=nullptr;ma_uint32 playbackCount=0,captureCount=0;
 if(ma_context_get_devices(&context,&playback,&playbackCount,&capture,&captureCount)==MA_SUCCESS){
  for(ma_uint32 i=0;i<captureCount;++i)result.inputs.emplace_back(capture[i].name);
  for(ma_uint32 i=0;i<playbackCount;++i)result.outputs.emplace_back(playback[i].name);
 }
 ma_context_uninit(&context);
 return result;
}
void LiveAudioEngine::configureProcessors(const Circuit& circuit){
 left_=std::make_unique<RealtimeCircuitProcessor>(circuit,double(sampleRate_));
 right_=std::make_unique<RealtimeCircuitProcessor>(circuit,double(sampleRate_));
}
void LiveAudioEngine::setLevels(float inputGainDb,float driveDb,float outputGainDb,bool bypass,int inputMode) noexcept{
 inputGain_.store(dbToGain(inputGainDb),std::memory_order_relaxed);
 driveGain_.store(dbToGain(driveDb),std::memory_order_relaxed);
 outputGain_.store(dbToGain(outputGainDb),std::memory_order_relaxed);
 bypass_.store(bypass,std::memory_order_relaxed);
 inputMode_.store(std::clamp(inputMode,0,2),std::memory_order_relaxed);
}
void LiveAudioEngine::dataCallback(ma_device* device,void* output,const void* input,unsigned frameCount){
 auto* self=static_cast<LiveAudioEngine*>(device->pUserData);
 if(self)self->process(static_cast<float*>(output),static_cast<const float*>(input),frameCount);
}
void LiveAudioEngine::processFrames(float* output,const float* input,unsigned frameCount) noexcept{
 if(!output)return;
 try{
  const float inputGain=inputGain_.load(std::memory_order_relaxed);
  const float drive=driveGain_.load(std::memory_order_relaxed);
  const float outputGain=outputGain_.load(std::memory_order_relaxed);
  const bool bypass=bypass_.load(std::memory_order_relaxed);
  const int inputMode=inputMode_.load(std::memory_order_relaxed);
  float inPeak=0.f,outPeak=0.f;
  for(unsigned i=0;i<frameCount;++i){
   float rawL=input?input[i*2]:0.f;
   float rawR=input?input[i*2+1]:rawL;
   if(inputMode==1)rawR=rawL;
   else if(inputMode==2)rawL=rawR;
   const float l=std::clamp(rawL*inputGain,-8.f,8.f);
   const float r=std::clamp(rawR*inputGain,-8.f,8.f);
   inPeak=std::max(inPeak,std::max(std::abs(l),std::abs(r)));
   const float drivenL=std::clamp(l*drive,-8.f,8.f);
   const float drivenR=std::clamp(r*drive,-8.f,8.f);
   const float wetL=left_?left_->process(drivenL):0.f;
   const float wetR=right_?right_->process(drivenR):0.f;
   const float yl=(bypass?l:wetL)*outputGain;
   const float yr=(bypass?r:wetR)*outputGain;
   output[i*2]=std::clamp(yl,-1.f,1.f);
   output[i*2+1]=std::clamp(yr,-1.f,1.f);
   outPeak=std::max(outPeak,std::max(std::abs(output[i*2]),std::abs(output[i*2+1])));
  }
  inputPeak_.store(inPeak,std::memory_order_relaxed);
  outputPeak_.store(outPeak,std::memory_order_relaxed);
 }catch(...){
  faulted_.store(true,std::memory_order_relaxed);
  std::fill(output,output+size_t(frameCount)*2,0.f);
 }
}
void LiveAudioEngine::process(float* output,const float* input,unsigned frameCount) noexcept{
 if(!fileMode_.load(std::memory_order_relaxed)){processFrames(output,input,frameCount);return;}
 if(!output||!impl_||!impl_->decoderInitialized){if(output)std::fill(output,output+size_t(frameCount)*2,0.f);return;}
 unsigned done=0;
 while(done<frameCount){
  const unsigned chunk=std::min<unsigned>(frameCount-done,8192);
  ma_uint64 got=0;
  ma_decoder_read_pcm_frames(&impl_->decoder,impl_->scratch.data(),chunk,&got);
  if(got<chunk){
   if(fileLoop_.load(std::memory_order_relaxed)){
    ma_decoder_seek_to_pcm_frame(&impl_->decoder,0);
    ma_uint64 extra=0;
    ma_decoder_read_pcm_frames(&impl_->decoder,impl_->scratch.data()+size_t(got)*2,chunk-got,&extra);
    got+=extra;
   }else fileEnded_.store(true,std::memory_order_relaxed);
  }
  if(got<chunk)std::fill(impl_->scratch.begin()+size_t(got)*2,impl_->scratch.begin()+size_t(chunk)*2,0.f);
  processFrames(output+size_t(done)*2,impl_->scratch.data(),chunk);
  done+=chunk;
 }
}
void LiveAudioEngine::start(const Circuit& circuit,int inputDevice,int outputDevice){
 stop();
 faulted_.store(false);fileMode_.store(false);fileEnded_.store(false);fileLoop_.store(false);
 inputPeak_.store(0);outputPeak_.store(0);configureProcessors(circuit);
 if(ma_context_init(kBackends,1,nullptr,&impl_->context)!=MA_SUCCESS)throw std::runtime_error("Nie można zainicjować WASAPI");
 impl_->contextInitialized=true;
 ma_device_info *playback=nullptr,*capture=nullptr;ma_uint32 playbackCount=0,captureCount=0;
 if(ma_context_get_devices(&impl_->context,&playback,&playbackCount,&capture,&captureCount)!=MA_SUCCESS){
  stop();throw std::runtime_error("Nie można pobrać listy urządzeń audio");
 }
 ma_device_config config=ma_device_config_init(ma_device_type_duplex);
 config.capture.format=ma_format_f32;config.capture.channels=channels_;
 config.playback.format=ma_format_f32;config.playback.channels=channels_;
 if(inputDevice>=0&&ma_uint32(inputDevice)<captureCount)config.capture.pDeviceID=&capture[inputDevice].id;
 if(outputDevice>=0&&ma_uint32(outputDevice)<playbackCount)config.playback.pDeviceID=&playback[outputDevice].id;
 config.sampleRate=sampleRate_;config.dataCallback=&LiveAudioEngine::dataCallback;config.pUserData=this;
 config.periodSizeInFrames=128;config.periods=3;
 if(ma_device_init(&impl_->context,&config,&impl_->device)!=MA_SUCCESS){stop();throw std::runtime_error("Nie można otworzyć wybranego wejścia/wyjścia WASAPI");}
 impl_->initialized=true;
 if(ma_device_start(&impl_->device)!=MA_SUCCESS){stop();throw std::runtime_error("Nie można uruchomić strumienia audio WASAPI");}
 running_.store(true);
}
void LiveAudioEngine::startFile(const Circuit& circuit,const std::filesystem::path& path,bool loop,int outputDevice){
 stop();
 faulted_.store(false);fileMode_.store(true);fileEnded_.store(false);fileLoop_.store(loop);
 inputPeak_.store(0);outputPeak_.store(0);configureProcessors(circuit);
 auto decoderConfig=ma_decoder_config_init(ma_format_f32,channels_,sampleRate_);
 if(ma_decoder_init_file_w(path.c_str(),&decoderConfig,&impl_->decoder)!=MA_SUCCESS){
  left_.reset();right_.reset();fileMode_.store(false);throw std::runtime_error("Nie można otworzyć pliku audio MP3/WAV");
 }
 impl_->decoderInitialized=true;
 if(ma_context_init(kBackends,1,nullptr,&impl_->context)!=MA_SUCCESS){stop();throw std::runtime_error("Nie można zainicjować WASAPI");}
 impl_->contextInitialized=true;
 ma_device_info *playback=nullptr,*capture=nullptr;ma_uint32 playbackCount=0,captureCount=0;
 ma_context_get_devices(&impl_->context,&playback,&playbackCount,&capture,&captureCount);
 ma_device_config config=ma_device_config_init(ma_device_type_playback);
 config.playback.format=ma_format_f32;config.playback.channels=channels_;
 if(outputDevice>=0&&ma_uint32(outputDevice)<playbackCount)config.playback.pDeviceID=&playback[outputDevice].id;
 config.sampleRate=sampleRate_;config.dataCallback=&LiveAudioEngine::dataCallback;config.pUserData=this;
 config.periodSizeInFrames=128;config.periods=3;
 if(ma_device_init(&impl_->context,&config,&impl_->device)!=MA_SUCCESS){stop();throw std::runtime_error("Nie można otworzyć wyjścia audio WASAPI dla magnetofonu");}
 impl_->initialized=true;
 if(ma_device_start(&impl_->device)!=MA_SUCCESS){stop();throw std::runtime_error("Nie można uruchomić odtwarzania magnetofonu");}
 running_.store(true);
}
void LiveAudioEngine::rewindFile(){
 if(!impl_||!impl_->decoderInitialized)throw std::runtime_error("Magnetofon nie ma załadowanego pliku");
 if(ma_decoder_seek_to_pcm_frame(&impl_->decoder,0)!=MA_SUCCESS)throw std::runtime_error("Nie można przewinąć pliku audio");
 fileEnded_.store(false);if(left_)left_->reset();if(right_)right_->reset();
}
void LiveAudioEngine::setFileLoop(bool loop) noexcept{fileLoop_.store(loop,std::memory_order_relaxed);}
void LiveAudioEngine::stop() noexcept{
 running_.store(false);
 if(impl_&&impl_->initialized){ma_device_uninit(&impl_->device);impl_->initialized=false;}
 if(impl_&&impl_->decoderInitialized){ma_decoder_uninit(&impl_->decoder);impl_->decoderInitialized=false;}
 if(impl_&&impl_->contextInitialized){ma_context_uninit(&impl_->context);impl_->contextInitialized=false;}
 left_.reset();right_.reset();fileMode_.store(false);fileEnded_.store(false);
}
std::string LiveAudioEngine::backendName() const{return "WASAPI / miniaudio 0.11.26";}
}
#endif
