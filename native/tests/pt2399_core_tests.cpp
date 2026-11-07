#include "pt2399_core.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(){
 using vbal::Pt2399Core;
 const auto nearly=[](double a,double b,double tolerance){return std::abs(a-b)<tolerance;};
 assert(nearly(Pt2399Core::delaySeconds(27600),0.342,1e-9));
 assert(nearly(Pt2399Core::delaySeconds(10500),0.151,1e-9));
 assert(nearly(Pt2399Core::delaySeconds(3400),0.0681,1e-9));
 assert(nearly(Pt2399Core::delaySeconds(2000),0.0523,1e-9));
 assert(Pt2399Core::delaySeconds(7600)>Pt2399Core::delaySeconds(6800));
 bool rejected=false;try{(void)Pt2399Core::delaySeconds(-1);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected);
 constexpr double rate=48000;
 auto firstArrival=[&](double resistor){
  Pt2399Core delay(rate);
  std::size_t first=0;
  for(std::size_t i=0;i<20000;i++){
   const float value=delay.step(i==0?1.f:0.f,resistor,5.);
   assert(std::isfinite(value));
   if(first==0&&std::abs(value)>0.001f)first=i;
  }
  return first;
 };
 const auto fast=firstArrival(2000),slow=firstArrival(10500);
 assert(fast>2400&&fast<2650);
 assert(slow>7100&&slow<7400);
 assert(slow>fast+4400);
 Pt2399Core unpowered(rate);
 for(int i=0;i<10000;i++)assert(unpowered.step(i==0?1.f:0.f,2700,0.f)==0.f);
 assert(!unpowered.initialized());
 for(int i=0;i<15000;i++)unpowered.step(0.1f,2000,5.f);
 assert(unpowered.initialized());
 assert(unpowered.step(0.1f,2000,4.f)==0.f);
 assert(!unpowered.initialized());
 std::cout<<"PASS PT2399 time-vs-R, impulse onset, no-output without 5V, power-reset\n";
}
