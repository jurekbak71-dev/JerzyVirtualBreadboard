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
   : rate_(sampleRate),ring_(allocateSize(sampleRate),0.f) {}
 static double delaySeconds(double ohms){
  static constexpr std::array<std::array<double,2>,28> curve{{
   {{0.5,31.3}},{{288.,32.6}},{{519.,34.4}},{{723.,36.6}},
   {{894.,38.5}},{{1080.,40.6}},{{1280.,43.}},{{1470.,45.8}},
   {{1670.,48.1}},{{2000.,52.3}},{{2400.,56.6}},{{2800.,61.6}},
   {{3400.,68.1}},{{4000.,75.9}},{{4500.,81.}},
   {{4900.,86.3}},{{5400.,92.2}},{{5800.,97.1}},
   {{6400.,104.3}},{{7200.,113.7}},{{8200.,124.1}},
   {{9200.,136.6}},{{10500.,151.}},{{12100.,171.}},
   {{14300.,196.}},{{17200.,228.}},{{21300.,273.}},
   {{27600.,342.}}
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
 }
 // Differential AC input, physical volts. Return differential AC output.
 // The external SPICE/MNA network is responsible for LPF, gain and feedback.
 float step(float input,double pin6Ohms,double supplyVolts){
  if(!std::isfinite(input)||!std::isfinite(pin6Ohms)||!std::isfinite(supplyVolts))
   throw std::invalid_argument("Invalid PT2399 sample or supply");
  if(supplyVolts<4.5||supplyVolts>5.5){reset();return 0.f;}
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
};
}
