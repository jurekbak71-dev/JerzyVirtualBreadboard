#include "audio.hpp"
#include "circuit.hpp"
#include "spice_render.hpp"
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
int main(int argc,char** argv){
 if(argc!=4){std::cerr<<"Usage: vbal_render_graph input.wav elements.txt output.wav\n";return 2;}
 try{
  std::ifstream file(argv[2],std::ios::binary);
  if(!file)throw std::runtime_error("Unable to read editor circuit");
  std::string definition((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
  auto graph=vbal::parseEditorSpiceElements(definition);
  auto audio=vbal::readPcmWav(argv[1]);
  auto result=vbal::renderGraphWithNgspice(audio,graph);
  vbal::writePcm16Wav(argv[3],result);
  std::cout<<"PASS: graph rendered to WAV\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
