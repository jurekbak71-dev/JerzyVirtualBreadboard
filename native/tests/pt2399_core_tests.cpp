#include "pt2399_core.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(){
 using vbal::Pt2399Core;
 const auto nearly=[](double a,double b,double tolerance){return std::abs(a-b)<tolerance;};
 assert(nearly(Pt2399Core::delaySeconds(27100),0.344,1e-9));
 assert(nearly(Pt2399Core::delaySeconds(9200),0.150,1e-9));
 assert(nearly(Pt2399Core::delaySeconds(11740),0.170,1e-9));
 assert(nearly(Pt2399Core::delaySeconds(2050),0.0568,1e-9));
 assert(nearly(Pt2399Core::delaySeconds(3760),0.07504,1e-9));
 assert(Pt2399Core::delaySeconds(7600)>Pt2399Core::delaySeconds(6800));
 bool rejected=false;try{(void)Pt2399Core::delaySeconds(-1);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected);
 constexpr double rate=48000;
 auto firstArrival=[&](double resistor){
  Pt2399Core delay(rate);
  for(std::size_t i=0;i<19200;i++)assert(delay.step(0.f,resistor,5.)==0.f);
  assert(delay.startupMuteRemainingSeconds()==0.);
  std::size_t first=0;
  for(std::size_t i=0;i<20000;i++){
   const float value=delay.step(i==0?1.f:0.f,resistor,5.);
   assert(std::isfinite(value));
   if(first==0&&std::abs(value)>0.001f)first=i;
  }
  return first;
 };
 const auto fast=firstArrival(2000),slow=firstArrival(10500);
 assert(fast>2600&&fast<2900);
 assert(slow>7600&&slow<8000);
 assert(slow>fast+4500);
 Pt2399Core unpowered(rate);
 for(int i=0;i<10000;i++)assert(unpowered.step(i==0?1.f:0.f,2700,0.f)==0.f);
 assert(!unpowered.initialized());
 for(int i=0;i<19199;i++)assert(unpowered.step(0.1f,2000,5.f)==0.f);
 assert(!unpowered.initialized());
 assert(unpowered.startupMuteRemainingSeconds()>0.);
 for(int i=0;i<10000;i++)unpowered.step(0.1f,2000,5.f);
 assert(unpowered.initialized());
 assert(unpowered.step(0.1f,2000,4.f)==0.f);
 assert(!unpowered.initialized());
 assert(unpowered.startupMuteRemainingSeconds()>0.399);
 for(int i=0;i<19200;i++)assert(unpowered.step(0.1f,2000,5.f)==0.f);
 assert(!unpowered.initialized());
 std::cout<<"PASS PT2399 V1.9 R-vs-time, power-on mute, impulse onset and power-cycle reset\\n";
}
