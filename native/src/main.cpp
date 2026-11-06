#include "audio.hpp"
#include <exception>
#include <iostream>
#include <string>
int main(int argc,char**argv){
 if(argc!=4){std::cerr<<"Usage: vbal_rc input_pcm.wav output.wav R_ohms,C_farads (e.g. 10000,1e-8)\n";return 2;}
 try{ auto p=std::string(argv[3]);auto comma=p.find(',');if(comma==std::string::npos) throw std::runtime_error("Expected R,C");auto r=std::stod(p.substr(0,comma)),c=std::stod(p.substr(comma+1));auto a=vbal::readPcmWav(argv[1]);vbal::rcLowpass(a,r,c);vbal::writePcm16Wav(argv[2],a);std::cout<<"Rendered "<<a.samples.size()/a.channels<<" frames at "<<a.sampleRate<<" Hz\n";return 0;}
 catch(const std::exception&e){std::cerr<<"Error: "<<e.what()<<"\n";return 1;}
}
