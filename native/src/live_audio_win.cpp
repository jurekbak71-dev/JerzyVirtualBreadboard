#ifdef _WIN32
#include "live_audio_win.hpp"
#include "realtime_circuit.hpp"
#include <miniaudio.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace vbal {
struct LiveAudioEngine::Impl {
 ma_device device{};
 bool initialized=false;
 ma_decoder decoder{};
 bool decoderInitialized=false;
 std::vector<float> scratch=std::vector<float>(8192*2);
};
LiveAudioEngine::LiveAudioEngine():impl_(std::make_unique<Impl>()){}
LiveAudioEngine::~LiveAudioEngine(){stop();}
void LiveAudioEngine::configureProcessors(const Circuit& circuit){
 left_=std::make_unique<RealtimeCircuitProcessor>(circuit,double(sampleRate_));
 right_=std::make_unique<RealtimeCircuitProcessor>(circuit,double(sampleRate_));
}
void LiveAudioEngine::dataCallback(ma_device* device,void* output,const void* input,unsigned frameCount){
 auto* self=static_cast<LiveAudioEngine*>(device->pUserData);
 if(self)self->process(static_cast<float*>(output),static_cast<const float*>(input),frameCount);
}
void LiveAudioEngine::processFrames(float* output,const float* input,unsigned frameCount) noexcept{
 if(!output)return;
 try{
  float inPeak=0.f,outPeak=0.f;
  for(unsigned i=0;i<frameCount;++i){
   const float l=input?input[i*2]:0.f;
   const float r=input?input[i*2+1]:l;
   inPeak=std::max(inPeak,std::max(std::abs(l),std::abs(r)));
   const float yl=left_?left_->process(l):0.f;
   const float yr=right_?right_->process(r):0.f;
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
   }else{
    fileEnded_.store(true,std::memory_order_relaxed);
   }
  }
  if(got<chunk)std::fill(impl_->scratch.begin()+size_t(got)*2,impl_->scratch.begin()+size_t(chunk)*2,0.f);
  processFrames(output+size_t(done)*2,impl_->scratch.data(),chunk);
  done+=chunk;
 }
}
void LiveAudioEngine::start(const Circuit& circuit){
 stop();
 faulted_.store(false);fileMode_.store(false);fileEnded_.store(false);fileLoop_.store(false);
 inputPeak_.store(0);outputPeak_.store(0);
 configureProcessors(circuit);
 ma_device_config config=ma_device_config_init(ma_device_type_duplex);
 config.capture.format=ma_format_f32; config.capture.channels=channels_;
 config.playback.format=ma_format_f32; config.playback.channels=channels_;
 config.sampleRate=sampleRate_;
 config.dataCallback=&LiveAudioEngine::dataCallback; config.pUserData=this;
 config.periodSizeInFrames=128; config.periods=3;
 const ma_backend backends[]={ma_backend_wasapi};
 const ma_result init=ma_device_init_ex(backends,1,nullptr,&config,&impl_->device);
 if(init!=MA_SUCCESS){left_.reset();right_.reset();throw std::runtime_error("Nie można otworzyć domyślnego wejścia/wyjścia audio WASAPI");}
 impl_->initialized=true;
 if(ma_device_start(&impl_->device)!=MA_SUCCESS){
  ma_device_uninit(&impl_->device);impl_->initialized=false;left_.reset();right_.reset();
  throw std::runtime_error("Nie można uruchomić strumienia audio WASAPI");
 }
 running_.store(true);
}
void LiveAudioEngine::startFile(const Circuit& circuit,const std::filesystem::path& path,bool loop){
 stop();
 faulted_.store(false);fileMode_.store(true);fileEnded_.store(false);fileLoop_.store(loop);
 inputPeak_.store(0);outputPeak_.store(0);
 configureProcessors(circuit);
 auto decoderConfig=ma_decoder_config_init(ma_format_f32,channels_,sampleRate_);
 if(ma_decoder_init_file_w(path.c_str(),&decoderConfig,&impl_->decoder)!=MA_SUCCESS){
  left_.reset();right_.reset();fileMode_.store(false);
  throw std::runtime_error("Nie można otworzyć pliku audio MP3/WAV");
 }
 impl_->decoderInitialized=true;
 ma_device_config config=ma_device_config_init(ma_device_type_playback);
 config.playback.format=ma_format_f32; config.playback.channels=channels_;
 config.sampleRate=sampleRate_;
 config.dataCallback=&LiveAudioEngine::dataCallback; config.pUserData=this;
 config.periodSizeInFrames=128; config.periods=3;
 const ma_backend backends[]={ma_backend_wasapi};
 if(ma_device_init_ex(backends,1,nullptr,&config,&impl_->device)!=MA_SUCCESS){
  ma_decoder_uninit(&impl_->decoder);impl_->decoderInitialized=false;left_.reset();right_.reset();fileMode_.store(false);
  throw std::runtime_error("Nie można otworzyć wyjścia audio WASAPI dla magnetofonu");
 }
 impl_->initialized=true;
 if(ma_device_start(&impl_->device)!=MA_SUCCESS){
  ma_device_uninit(&impl_->device);impl_->initialized=false;
  ma_decoder_uninit(&impl_->decoder);impl_->decoderInitialized=false;left_.reset();right_.reset();fileMode_.store(false);
  throw std::runtime_error("Nie można uruchomić odtwarzania magnetofonu");
 }
 running_.store(true);
}
void LiveAudioEngine::rewindFile(){
 if(!impl_||!impl_->decoderInitialized)throw std::runtime_error("Magnetofon nie ma załadowanego pliku");
 if(ma_decoder_seek_to_pcm_frame(&impl_->decoder,0)!=MA_SUCCESS)throw std::runtime_error("Nie można przewinąć pliku audio");
 fileEnded_.store(false);
 if(left_)left_->reset(); if(right_)right_->reset();
}
void LiveAudioEngine::setFileLoop(bool loop) noexcept{fileLoop_.store(loop,std::memory_order_relaxed);}
void LiveAudioEngine::stop() noexcept{
 running_.store(false);
 if(impl_&&impl_->initialized){ma_device_uninit(&impl_->device);impl_->initialized=false;}
 if(impl_&&impl_->decoderInitialized){ma_decoder_uninit(&impl_->decoder);impl_->decoderInitialized=false;}
 left_.reset();right_.reset();
 fileMode_.store(false);fileEnded_.store(false);
}
std::string LiveAudioEngine::backendName() const{return "WASAPI / miniaudio 0.11.26";}
}
#endif
