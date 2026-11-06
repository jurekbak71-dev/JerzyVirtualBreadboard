#include "audio.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
namespace vbal {
namespace {
void need(bool ok, const char* why) { if(!ok) throw std::runtime_error(why); }
uint16_t r16(std::istream& in){ unsigned char b[2]{}; in.read(reinterpret_cast<char*>(b),2); need(bool(in),"Truncated WAV"); return uint16_t(b[0]) | uint16_t(b[1])<<8; }
uint32_t r32(std::istream& in){ return uint32_t(r16(in)) | uint32_t(r16(in))<<16; }
void w16(std::ostream& o,uint16_t v){ o.put(char(v&255)); o.put(char(v>>8)); }
void w32(std::ostream& o,uint32_t v){ w16(o,uint16_t(v)); w16(o,uint16_t(v>>16)); }
}
Audio readPcmWav(const std::filesystem::path& path){
 std::ifstream in(path,std::ios::binary); need(bool(in),"Cannot open WAV");
 char tag[4]{}; in.read(tag,4); need(std::string(tag,4)=="RIFF","Not RIFF WAV");
 const auto riffSize=r32(in); (void)riffSize;
 in.read(tag,4); need(std::string(tag,4)=="WAVE","Not WAVE");
 Audio a; bool fmt=false, data=false; std::vector<char> pcm; uint16_t sampleBits=0;
 while(in.read(tag,4)) {
   auto size=r32(in); auto start=in.tellg(); need(start>=0,"Invalid WAV chunk");
   if(std::string(tag,4)=="fmt "){
     need(size>=16,"Invalid fmt chunk"); auto format=r16(in); a.channels=r16(in); a.sampleRate=r32(in);
     auto byteRate=r32(in); auto align=r16(in); auto bits=r16(in);
     need(format==1 && (bits==16 || bits==24 || bits==32),"Only PCM16/24/32 WAV is supported"); sampleBits=bits;
     need(a.channels>=1 && a.channels<=8 && a.sampleRate>=8000 && a.sampleRate<=384000,"Invalid audio format");
     need(align==a.channels*(bits/8) && byteRate==a.sampleRate*align,"Invalid WAV byte rate"); fmt=true;
   } else if(std::string(tag,4)=="data"){
     need(size>0 && size<=256u*1024u*1024u,"Invalid WAV data size");
     pcm.resize(size); in.read(pcm.data(),size); need(bool(in),"Truncated WAV data"); data=true;
   }
   in.clear(); in.seekg(start+std::streamoff(size+(size&1))); need(bool(in),"Invalid WAV chunk offset");
 }
 need(fmt && data && pcm.size()%(a.channels*(sampleBits/8))==0,"WAV missing fmt/data or invalid sample count");
 const auto width=size_t(sampleBits/8); a.samples.resize(pcm.size()/width);
 for(size_t i=0;i<a.samples.size();++i){ uint32_t v=0; for(size_t j=0;j<width;++j) v|=uint32_t(uint8_t(pcm[i*width+j]))<<(8*j); const auto shift=32-sampleBits; auto signedSample=static_cast<int32_t>(v<<shift)>>shift; a.samples[i]=float(double(signedSample)/std::ldexp(1.,sampleBits-1)); }
 return a;
}
void writePcm16Wav(const std::filesystem::path& path,const Audio& a){
 need(a.channels>=1 && a.channels<=8 && a.sampleRate>=8000 && a.sampleRate<=384000 && !a.samples.empty() && a.samples.size()%a.channels==0,"Invalid audio buffer");
 need(a.samples.size()<=((std::numeric_limits<uint32_t>::max()-36)/2),"WAV too large");
 std::ofstream out(path,std::ios::binary); need(bool(out),"Cannot create WAV");
 auto bytes=uint32_t(a.samples.size()*2); out.write("RIFF",4); w32(out,36+bytes);out.write("WAVEfmt ",8);w32(out,16);w16(out,1);w16(out,a.channels);w32(out,a.sampleRate);w32(out,a.sampleRate*a.channels*2);w16(out,a.channels*2);w16(out,16);out.write("data",4);w32(out,bytes);
 for(float x:a.samples){need(std::isfinite(x),"Nonfinite audio sample"); auto clamped=std::clamp(double(x),-1.,1.); auto scaled=long(std::round(clamped*32767.)); w16(out,uint16_t(static_cast<int16_t>(scaled))); }
 need(bool(out),"WAV write failed");
}
void rcLowpass(Audio& a,double resistanceOhms,double capacitanceFarads){
 need(std::isfinite(resistanceOhms) && resistanceOhms>0 && std::isfinite(capacitanceFarads) && capacitanceFarads>0,"RC values must be positive and finite");
 need(a.sampleRate>=8000 && a.sampleRate<=384000 && a.channels>=1 && a.channels<=8 && !a.samples.empty() && a.samples.size()%a.channels==0,"Invalid audio buffer");
 double alpha=std::exp(-1./(double(a.sampleRate)*resistanceOhms*capacitanceFarads)); std::vector<double> prev(a.channels,0.);
 for(size_t i=0;i<a.samples.size();++i){need(std::isfinite(a.samples[i]),"Nonfinite audio sample"); auto ch=i%a.channels; prev[ch]=alpha*prev[ch]+(1.-alpha)*a.samples[i]; a.samples[i]=float(prev[ch]);}
}
}
