#ifdef _WIN32
#include "live_audio_win.hpp"
#include "realtime_circuit.hpp"
#include <miniaudio.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace vbal {
struct LiveAudioEngine::Impl { ma_device device{}; bool initialized=false; };
LiveAudioEngine::LiveAudioEngine():impl_(std::make_unique<Impl>()){}
LiveAudioEngine::~LiveAudioEngine(){stop();}
void LiveAudioEngine::dataCallback(ma_device* device,void* output,const void* input,unsigned frameCount){
 auto* self=static_cast<LiveAudioEngine*>(device->pUserData);
 if(self)self->process(static_cast<float*>(output),static_cast<const float*>(input),frameCount);
}
void LiveAudioEngine::process(float* output,const float* input,unsigned frameCount) noexcept{
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
void LiveAudioEngine::start(const Circuit& circuit){
 stop();
 faulted_.store(false);inputPeak_.store(0);outputPeak_.store(0);
 left_=std::make_unique<RealtimeCircuitProcessor>(circuit,double(sampleRate_));
 right_=std::make_unique<RealtimeCircuitProcessor>(circuit,double(sampleRate_));
 ma_device_config config=ma_device_config_init(ma_device_type_duplex);
 config.capture.format=ma_format_f32;
 config.capture.channels=channels_;
 config.playback.format=ma_format_f32;
 config.playback.channels=channels_;
 config.sampleRate=sampleRate_;
 config.dataCallback=&LiveAudioEngine::dataCallback;
 config.pUserData=this;
 config.periodSizeInFrames=128;
 config.periods=3;
 const ma_backend backends[]={ma_backend_wasapi};
 const ma_result init=ma_device_init_ex(backends,1,nullptr,&config,&impl_->device);
 if(init!=MA_SUCCESS){
  left_.reset();right_.reset();
  throw std::runtime_error("Nie można otworzyć domyślnego wejścia/wyjścia audio WASAPI");
 }
 impl_->initialized=true;
 if(ma_device_start(&impl_->device)!=MA_SUCCESS){
  ma_device_uninit(&impl_->device);impl_->initialized=false;left_.reset();right_.reset();
  throw std::runtime_error("Nie można uruchomić strumienia audio WASAPI");
 }
 running_.store(true);
}
void LiveAudioEngine::stop() noexcept{
 running_.store(false);
 if(impl_&&impl_->initialized){ma_device_uninit(&impl_->device);impl_->initialized=false;}
 left_.reset();right_.reset();
}
std::string LiveAudioEngine::backendName() const{return "WASAPI / miniaudio 0.11.26";}
}
#endif
