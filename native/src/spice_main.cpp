#include "audio.hpp"
#include "spice.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc, char** argv) {
 if(argc!=5) { std::cerr<<"Usage: vbal_spice input.wav output.cir R_ohms,C_farads channel_index\n";return 2; }
 try {
  auto parameters=std::string(argv[3]);
  auto comma=parameters.find(',');
  if(comma==std::string::npos) throw std::invalid_argument("Expected resistance,capacitance");
  double r=std::stod(parameters.substr(0,comma));
  double c=std::stod(parameters.substr(comma+1));
  auto channel=std::stoul(argv[4]);
  auto audio=vbal::readPcmWav(argv[1]);
  std::string netlist=vbal::makeRcSpiceNetlist(audio,r,c,channel);
  std::ofstream out(argv[2], std::ios::binary);
  if(!out) throw std::runtime_error("Cannot create netlist");
  out<<netlist;
  if(!out) throw std::runtime_error("Cannot write netlist");
  std::cout<<"Generated "<<std::filesystem::path(argv[2]).string()<<" for "<<audio.samples.size()/audio.channels<<" frames\n";
  return 0;
 } catch(const std::exception& e) {std::cerr<<"Error: "<<e.what()<<"\n";return 1;}
}
