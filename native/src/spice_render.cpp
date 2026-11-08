#include "spice_render.hpp"
#include "spice.hpp"
#include "circuit.hpp"
#include "realtime_circuit.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <system_error>
namespace vbal {
std::vector<SpicePoint> parseSpiceWrdata(const std::string& text) {
 std::vector<SpicePoint> out; std::istringstream input(text);std::string line;
 while(std::getline(input,line)){
  if(line.empty()||line[0]=='#') continue;
  std::istringstream row(line);double time=0.,voltage=0.;
  if(!(row>>time)) continue;
  if(!(row>>voltage)) throw std::runtime_error("Malformed ngspice voltage value");
  if(!std::isfinite(time)||!std::isfinite(voltage)||time<0) throw std::runtime_error("Invalid ngspice data");
  if(!out.empty()&&time<out.back().time) throw std::runtime_error("Out-of-order ngspice samples");
  if(!out.empty()&&time==out.back().time) out.back().voltage=voltage;
  else out.push_back({time,voltage});
 }
 if(out.empty()) throw std::runtime_error("ngspice output has no samples");
 return out;
}
std::vector<float> resampleSpiceOutput(const std::vector<SpicePoint>& points,unsigned sampleRate,std::size_t frames){
 if(points.empty()||sampleRate==0||frames==0) throw std::runtime_error("Invalid resampling request");
 const double dt=1.0/sampleRate,epsilon=dt*1e-5;
 if(points.front().time>epsilon||points.back().time+epsilon<(frames-1)*dt) throw std::runtime_error("ngspice simulation does not cover requested sample range");
 std::vector<float> out;out.reserve(frames);size_t i=0;
 for(size_t n=0;n<frames;++n){
  double t=n*dt;
  while(i+1<points.size()&&points[i+1].time<=t) ++i;
  double v=points[i].voltage;
  if(i+1<points.size()&&points[i+1].time>t){double fraction=std::clamp((t-points[i].time)/(points[i+1].time-points[i].time),0.,1.);v+=(points[i+1].voltage-v)*fraction;}
  if(!std::isfinite(v)||std::abs(v)>1e12)throw std::runtime_error("Invalid voltage amplitude");
  out.push_back(static_cast<float>(v));
 }
 return out;
}
namespace {
struct TemporaryDirectory{
 std::filesystem::path dir;
 TemporaryDirectory(){
  auto root=std::filesystem::temp_directory_path();std::random_device rd;
  for(int attempt=0;attempt<20;++attempt){dir=root/("vbal-spice-"+std::to_string(rd())+"-"+std::to_string(attempt));std::error_code ec;if(std::filesystem::create_directory(dir,ec))return;}
  throw std::runtime_error("Cannot allocate ngspice workdir");
 }
 ~TemporaryDirectory(){std::error_code ec;std::filesystem::remove_all(dir,ec);}
};
std::string slurp(const std::filesystem::path& file){std::ifstream f(file,std::ios::binary);if(!f)throw std::runtime_error("ngspice output missing");return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
}
std::string writeFileSourceNetlist(const Audio& audio,double r,double c,unsigned channel,const std::filesystem::path& directory){
 const size_t frames=audio.samples.size()/audio.channels;
 if(frames>500000) throw std::runtime_error("ngspice offline limit: 500000 audio frames per render");
 const auto signalPath=directory/"signal.dat";
 std::ofstream samples(signalPath);if(!samples)throw std::runtime_error("Cannot create ngspice audio signal file");
 samples<<std::setprecision(17);
 for(size_t i=0;i<frames;++i){
  const auto v=audio.samples[i*audio.channels+channel];
  if(!std::isfinite(v))throw std::runtime_error("Nonfinite audio sample");
  samples<<double(i)/audio.sampleRate<<" "<<v<<"\n";
 }
 samples<<double(frames)/audio.sampleRate<<" "<<audio.samples[(frames-1)*audio.channels+channel]<<"\n";
 samples.close();if(!samples)throw std::runtime_error("Cannot write ngspice signal data");
 const double dt=1.0/audio.sampleRate;
 std::ostringstream netlist;netlist<<std::setprecision(17);
 netlist<<"* Virtual Breadboard audio signal from XSPICE filesource\n";
 netlist<<"ASrc %vd([in 0]) audiosrc\n";
 netlist<<".model audiosrc filesource (file=\"signal.dat\" amploffset=[0] amplscale=[1]\n";
 netlist<<"+ timeoffset=0 timescale=1 timerelative=false amplstep=false)\n";
 netlist<<"R1 in out "<<r<<"\nC1 out 0 "<<c<<"\n";
 netlist<<".control\nset wr_singlescale\nset wr_vecnames\ntran "<<dt<<" "<<frames*dt<<"\nwrdata rc_output.dat v(out)\n.endc\n.end\n";
 return netlist.str();
}
Audio renderGraphWithNgspice(const Audio& input,const Circuit& graph){
 validateCircuit(graph);
 // PT2399 is not supported by vanilla ngspice. Render this mixed-signal
 // circuit through the same explicit per-node MNA as live audio. Never claim
 // that a fake SPICE macro has modelled its internal digital memory.
 if(std::any_of(graph.parts.begin(),graph.parts.end(),[](const Part& p){return p.type==PartType::Pt2399||p.type==PartType::Mn3007||p.type==PartType::Mn3101;})){
  if(input.channels==0||input.channels>8||input.sampleRate<8000||input.sampleRate>384000||input.samples.empty()||input.samples.size()%input.channels)
   throw std::runtime_error("Invalid mixed-signal audio");
  Audio output=input;
  for(unsigned channel=0;channel<input.channels;++channel){
   RealtimeCircuitProcessor solver(graph,input.sampleRate);
   for(size_t i=0;i<input.samples.size()/input.channels;++i)
    output.samples[i*input.channels+channel]=solver.process(input.samples[i*input.channels+channel]);
  }
  return output;
 }

 if(input.channels==0||input.channels>8||input.samples.empty()||input.samples.size()%input.channels)throw std::runtime_error("Invalid audio");
 const size_t frames=input.samples.size()/input.channels;
 Audio output=input; std::fill(output.samples.begin(),output.samples.end(),0.f);
 TemporaryDirectory work;
 for(unsigned channel=0;channel<input.channels;++channel){
  auto netlist=(frames<=4096)?makeRcSpiceNetlist(input,10000.,1e-8,channel):writeFileSourceNetlist(input,10000.,1e-8,channel,work.dir);
  // Single source of truth for actual component topology and electrical values.
  const auto bodyStart=netlist.find("R1 in out ");
  const auto controlStart=netlist.find(".control");
  if(bodyStart==std::string::npos||controlStart==std::string::npos||controlStart<=bodyStart)
   throw std::runtime_error("Missing circuit section in SPICE deck");
  netlist.replace(bodyStart,controlStart-bodyStart,generateSpiceElements(graph));
  const auto file=work.dir/("channel-"+std::to_string(channel)+".cir");
  {std::ofstream f(file);if(!f)throw std::runtime_error("Cannot save ngspice input"); f<<netlist; if(!f)throw std::runtime_error("Cannot write ngspice input");}
#ifdef _WIN32
  const std::string cmd="cd /d \""+work.dir.string()+"\" && ngspice_con.exe -b \""+file.filename().string()+"\" > ngspice.log 2>&1";
#else
  const std::string cmd="cd \""+work.dir.string()+"\" && ngspice -b \""+file.filename().string()+"\" > ngspice.log 2>&1";
#endif
  if(std::system(cmd.c_str())!=0)throw std::runtime_error("ngspice failed: "+slurp(work.dir/"ngspice.log"));
  auto points=parseSpiceWrdata(slurp(work.dir/"rc_output.dat"));
  auto samples=resampleSpiceOutput(points,input.sampleRate,frames);
  for(size_t i=0;i<frames;++i)output.samples[i*input.channels+channel]=samples[i];
 }
 return output;
}
Audio renderCircuitWithNgspice(const Audio& input,double r,double c,CircuitMode mode){
 return renderGraphWithNgspice(input,makeRcCircuit(r,c,mode==CircuitMode::DiodeClipper));
}
Audio renderRcWithNgspice(const Audio& input,double r,double c){
 return renderCircuitWithNgspice(input,r,c,CircuitMode::RcLowpass);
}

}
