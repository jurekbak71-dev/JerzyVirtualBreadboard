#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace vbal {
// Isolated approximation of PT2399's internal mixed-signal delay path.
// This is NOT a transistor-level or manufacturer-certified model.
// PT2399: Princeton Technology, external oscillator resistor on pin 6;
// calibration: PT2399 data sheet "Resistor/Delay Time Values" table.
class Pt2399Core {
public:
 explicit Pt2399Core(double sampleRate)
   : rate_(sampleRate),ring_(allocateSize(sampleRate),0.f) { reset(); }
 static double delaySeconds(double ohms){
  // Princeton PT2399 V1.9 (March 2022), Table 1, R vs td.
  // The source appears to print 7504ms at 3.76k: interpreted as 75.04ms
  // because adjacent samples are 81ms at 4.13k and 67.8ms at 3k.
  // That ambiguous entry is explicitly tagged; measured silicon is needed.
  static constexpr std::array<std::array<double,2>,26> curve{{
   {{71.,33.9}},{{340.,36.1}},{{510.,38.2}},{{660.,40.3}},
   {{740.,42.2}},{{1130.,45.4}},{{1380.,47.8}},{{1700.,52.1}},
   {{2050.,56.8}},{{2530.,61.2}},{{3000.,67.8}},{{3760.,75.04}},
   {{4130.,81.}},{{4500.,85.}},{{5050.,91.}},{{5500.,96.}},
   {{5880.,104.}},{{6270.,113.}},{{6400.,124.}},{{6600.,135.}},
   {{9200.,150.}},{{11740.,170.}},{{14250.,194.}},
   {{16760.,227.}},{{22000.,274.}},{{27100.,344.}}
  }};
  if(!std::isfinite(ohms)||ohms<0.)throw std::invalid_argument("Invalid PT2399 pin-6 resistor");
  if(ohms<=curve.front()[0])return curve.front()[1]*.001;
  if(ohms>=curve.back()[0])return curve.back()[1]*.001;
  const auto it=std::lower_bound(curve.begin(),curve.end(),ohms,
    [](const auto& point,double r){return point[0]<r;});
  const auto& previous=*(it-1);
  const double blend=(ohms-previous[0])/(it->at(0)-previous[0]);
  return .001*(previous[1]+blend*(it->at(1)-previous[1]));
 }
 void reset() noexcept{
  std::fill(ring_.begin(),ring_.end(),0.f);
  write_=0;initialized_=false;lowpass_=0.f;
  startupSamplesRemaining_=static_cast<std::size_t>(std::ceil(rate_*kPowerOnMuteSeconds));
 }
 // Differential AC input, physical volts. Return differential AC output.
 // The external SPICE/MNA network is responsible for LPF, gain and feedback.
 float step(float input,double pin6Ohms,double supplyVolts){
  if(!std::isfinite(input)||!std::isfinite(pin6Ohms)||!std::isfinite(supplyVolts))
   throw std::invalid_argument("Invalid PT2399 sample or supply");
  if(supplyVolts<4.5||supplyVolts>5.5){reset();return 0.f;}
  // Startup auto-reset/mute is a configurable 400 ms approximation of the
  // power-on waveform (the datasheet does not specify a precise digital timer).
  if(startupSamplesRemaining_>0){--startupSamplesRemaining_;return 0.f;}
  const double samples=delaySeconds(pin6Ohms)*rate_;
  const double read=double(write_)+ring_.size()-samples;
  const double floored=std::floor(read);
  const auto idx=[&](long long offset){
   const auto size=static_cast<long long>(ring_.size());
   const auto wrapped=(offset%size+size)%size;
   return static_cast<std::size_t>(wrapped);
  };
  const float delayed=ring_[idx(static_cast<long long>(floored))]*(1.f-float(read-floored))
   +ring_[idx(static_cast<long long>(floored)+1)]*float(read-floored);
  ring_[write_]=input;
  if(++write_==ring_.size())write_=0;
  // Stable low-pass approximation to an internal reconstruction stage.
  const float coefficient=1.f-std::exp(-2.f*3.14159265f*5000.f/float(rate_));
  lowpass_+=coefficient*(delayed-lowpass_);
  initialized_=true;
  return lowpass_;
 }
 bool initialized() const noexcept{return initialized_;}
 double startupMuteRemainingSeconds() const noexcept{return double(startupSamplesRemaining_)/rate_;}
 static constexpr double kPowerOnMuteSeconds=0.400;
 private:
 static std::size_t allocateSize(double rate){
  if(!std::isfinite(rate)||rate<8000||rate>384000)
   throw std::invalid_argument("Unsupported PT2399 model sample rate");
  return static_cast<std::size_t>(std::ceil(rate*.45))+8;
 }
 double rate_;
 std::vector<float> ring_;
 std::size_t write_=0;
 float lowpass_=0.f;
 bool initialized_=false;
 std::size_t startupSamplesRemaining_=0;
};
}
