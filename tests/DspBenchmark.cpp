#include "../Source/FuzzEngine.h"
#include <chrono>
#include <iostream>
int main(){FuzzEngine engine;engine.prepare(48000);std::array<float,FuzzEngine::maxIR> ir{};for(int i=0;i<FuzzEngine::maxIR;++i)ir[static_cast<size_t>(i)]=std::exp(-i*.002f);engine.stageIR(ir.data(),FuzzEngine::maxIR);engine.beginBlock();FuzzEngine::Parameters p;p.feedback=.85f;float checksum=0;auto start=std::chrono::steady_clock::now();for(int i=0;i<48000;++i){float l=std::sin(i*.03f)*.1f,r=l;engine.process(l,r,p);checksum+=l+r;}double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cout<<"2048-tap stereo FIR, 48000 frames at 48kHz: "<<elapsed<<" seconds wall per audio second ("<<elapsed*100<<"% of one realtime core), checksum="<<checksum<<"\n";}
