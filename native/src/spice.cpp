#include "spice.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace vbal {
std::string makeRcSpiceNetlist(const Audio& audio,double r,double c,std::size_t channel){
 if(!std::isfinite(r)||!std::isfinite(c)||r<=0||c<=0) throw std::invalid_argument("Invalid RC value");
 if(audio.sampleRate<8000||audio.sampleRate>384000||audio.channels==0||audio.channels>8||channel>=audio.channels||audio.samples.empty()||audio.samples.size()%audio.channels!=0) throw std::invalid_argument("Invalid audio");
 const auto frames=audio.samples.size()/audio.channels;
 if(frames>4096) throw std::invalid_argument("PWL limit: at most 4096 frames; use streamed external source for long audio");
 for(auto v:audio.samples) if(!std::isfinite(v)) throw std::invalid_argument("Nonfinite audio");
 const double dt=1.0/audio.sampleRate;
 std::ostringstream out;out<<std::setprecision(17);
 out<<"* Virtual Breadboard Audio Lab ngspice RC offline verification\n";
 out<<"Vin in 0 PWL(";
 for(size_t frame=0;frame<frames;++frame){if(frame)out<<"\n+ ";out<<frame*dt<<' '<<audio.samples[frame*audio.channels+channel];}
 out<<"\n+ "<<frames*dt<<' '<<audio.samples[(frames-1)*audio.channels+channel]<<")\n";
 out<<"R1 in out "<<r<<"\nC1 out 0 "<<c<<"\n";
 out<<".control\nset wr_singlescale\nset wr_vecnames\ntran "<<dt<<' '<<frames*dt<<"\nwrdata rc_output.dat v(out)\n.endc\n.end\n";
 return out.str();
}
}
