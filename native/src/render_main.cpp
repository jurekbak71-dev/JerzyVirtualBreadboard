#include "audio.hpp"
#include "spice_render.hpp"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc,char**argv){
 if(argc!=4 && argc!=5){std::cerr<<"Usage: vbal_render_spice input.wav output.wav R_ohms,C_farads [rc|diode]\nRequires ngspice installed and on PATH. Supports <=4096 input frames.\n";return 2;}
 try{
  std::string values=argv[3];const auto comma=values.find(',');
  if(comma==std::string::npos)throw std::invalid_argument("Expected R,C");
  double r=std::stod(values.substr(0,comma)),c=std::stod(values.substr(comma+1));
  auto input=vbal::readPcmWav(argv[1]);
  const std::string mode=(argc==5)?argv[4]:"rc";
  if(mode!="rc"&&mode!="diode")throw std::invalid_argument("Circuit mode must be rc or diode");
  auto output=vbal::renderCircuitWithNgspice(input,r,c,mode=="diode"?vbal::CircuitMode::DiodeClipper:vbal::CircuitMode::RcLowpass);
  vbal::writePcm16Wav(argv[2],output);
  std::cout<<"Rendered "<<output.samples.size()/output.channels<<" audio frames using ngspice\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<"Error: "<<e.what()<<"\n";return 1;}
}
