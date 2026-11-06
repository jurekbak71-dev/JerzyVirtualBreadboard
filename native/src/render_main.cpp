#include "audio.hpp"
#include "spice_render.hpp"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc,char**argv){
 if(argc!=4){std::cerr<<"Usage: vbal_render_spice input.wav output.wav R_ohms,C_farads\nRequires ngspice installed and on PATH. Supports <=4096 input frames.\n";return 2;}
 try{
  std::string values=argv[3];const auto comma=values.find(',');
  if(comma==std::string::npos)throw std::invalid_argument("Expected R,C");
  double r=std::stod(values.substr(0,comma)),c=std::stod(values.substr(comma+1));
  auto input=vbal::readPcmWav(argv[1]);
  auto output=vbal::renderRcWithNgspice(input,r,c);
  vbal::writePcm16Wav(argv[2],output);
  std::cout<<"Rendered "<<output.samples.size()/output.channels<<" audio frames using ngspice\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<"Error: "<<e.what()<<"\n";return 1;}
}
