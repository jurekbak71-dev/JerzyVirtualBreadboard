#include "mn3007_core.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
int main(){
 using vbal::Mn3007Core;
 assert(std::abs(Mn3007Core::nominalDelaySeconds(100000)-0.00512)<1e-10);
 assert(std::abs(Mn3007Core::nominalDelaySeconds(40000)-0.0128)<1e-10);
 assert(std::abs(Mn3007Core::nominalDelaySeconds(10000)-0.0512)<1e-10);
 bool invalid=false;
 try{Mn3007Core::nominalDelaySeconds(400000);}catch(const std::invalid_argument&){invalid=true;}
 assert(invalid);
 constexpr double sr=48000.;
 auto impulseOnset=[&](double clock,double supply,double vgg){
  Mn3007Core bbd(sr);
  int hit=-1;float peak=0.f;
  for(int i=0;i<7000;++i){
   const float input=i==0?1.f:0.f;
   const float y=bbd.step(input,clock,supply,vgg,-7.5);
   if(std::abs(y)>0.1f){peak=std::max(peak,std::abs(y));if(hit<0)hit=i;}
  }
  return std::pair<int,float>{hit,peak};
 };
 auto fast=impulseOnset(100000.,-15.,-14.);
 auto slow=impulseOnset(10000.,-15.,-14.);
 auto dead=impulseOnset(40000.,0.,0.);
 assert(fast.first>200&&fast.first<270);
 assert(slow.first>2400&&slow.first<2600);
 assert(dead.first<0);
 Mn3007Core mod(sr);
 for(int i=0;i<10000;++i){
  const double clock=25000.+12000.*std::sin(2*3.14159265*i/(sr*0.5));
  const float y=mod.step(std::sin(i*0.01),clock,-15.,-14.,-7.5);
  assert(std::isfinite(y));
 }
 assert(mod.powered());
 assert(mod.step(0.3f,40000.,-3.,-2.,-7.5)==0.f);
 assert(!mod.powered());
 std::cout<<"PASS MN3007 1024 charge-transfer stages, clock-dependent delay and no audio without bias/supply\n";
}
