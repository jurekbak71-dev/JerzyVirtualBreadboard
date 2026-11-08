#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace vbal {
// Simplified, electrically gated switched-charge MN3007 BBD memory.
// 1024 stages, two charge transfers per two-phase clock cycle.
// This is NOT a MOSFET-level model; charge losses and clock feedthrough
// are approximated, and the input/output reconstruction filters must
// be physical circuit components external to this device.
class Mn3007Core {
public:
 static constexpr size_t Stages=1024;
 static double nominalDelaySeconds(double clockHz){
  if(!std::isfinite(clockHz)||clockHz<10000||clockHz>100000)
   throw std::invalid_argument("MN3007 clock must be 10-100kHz");
  return double(Stages)/(2.*clockHz);
 }
 explicit Mn3007Core(double sampleRate):rate_(sampleRate){
  if(!std::isfinite(rate_)||rate_<8000||rate_>384000)
   throw std::invalid_argument("Invalid MN3007 sample rate");
  reset();
 }
 void reset() noexcept{
  ring_.fill(0.f);position_=0;phase_=0.;output_=0.;powered_=false;
 }
 // Values are measured relative to physical GND:
 // supply: VDD nominally -15 V, gate: VGG nominally VDD+1 V.
 // bias: DC at the signal input, chosen by external circuit.
 float step(float vin,double clockHz,double vdd,double vgg,double bias){
  if(!std::isfinite(vin)||!std::isfinite(clockHz)||!std::isfinite(vdd)||
     !std::isfinite(vgg)||!std::isfinite(bias))
   throw std::invalid_argument("Invalid MN3007 electrical state");
  if(vdd>-13.8||vdd<-16.2||std::abs(vgg-vdd-1.)>1.5||
     clockHz<10000||clockHz>100000||bias>-4||bias<-11){
   reset();return 0.f;
  }
  powered_=true;
  phase_+=2*clockHz/rate_;
  const int transfers=static_cast<int>(phase_);
  phase_-=transfers;
  for(int n=0;n<transfers;++n){
   // Charge-storage imperfections depend on time in the bucket chain.
   const float previous=ring_[position_];
   ring_[position_]=std::clamp(vin-float(bias),-2.2f,2.2f);
   position_=(position_+1)%Stages;
   output_=previous*0.9985f;
  }
  return output_;
 }
 bool powered()const noexcept{return powered_;}
private:
 double rate_;
 std::array<float,Stages> ring_{};
 size_t position_=0;
 double phase_=0.;
 float output_=0.f;
 bool powered_=false;
};
}
