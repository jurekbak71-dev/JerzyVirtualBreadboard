#pragma once
#include <algorithm>
#include <cmath>

// Dynamic two-stage fuzz voice. "Si", "Ge", "LED" and "BJT" are distinct
// original software voicings, NOT component-accurate SPICE transistor circuits.
// A sag envelope reduces operating headroom after strong notes; BIAS shifts the
// nonlinear operating point; GATE controls starvation/sputter at low levels.
// State is channel-local and processing is allocation-free.
class AnalogFuzzStage
{
public:
    void reset() noexcept { supplySag=0.f; conduction=0.f; }

    float process(float signal, float bias, float gate, float voice,
                  float inputEnvelope, float sampleRate) noexcept
    {
        if (sampleRate != rateCached) configure(sampleRate);
        const float env=std::clamp(std::isfinite(inputEnvelope)?inputEnvelope:0.f,0.f,4.f);
        supplySag+=(env>supplySag?sagAttackPole:sagReleasePole)*(env-supplySag);
        const float operatingBias=std::clamp(bias,-1.f,1.f);
        const float choke=std::clamp(gate,0.f,1.f);
        const float biasMagnitude=std::abs(operatingBias);
        const float starvation=.0012f+.115f*choke*choke
                              +.027f*biasMagnitude*biasMagnitude*biasMagnitude;
        const float target=std::clamp((env-starvation*.45f)/(starvation*.9f),0.f,1.f);
        conduction+=(target>conduction?gateAttackPole:gateReleasePole)*(target-conduction);
        // Charge starvation has a slower recovery than attack; unlike a hard
        // threshold this follows the envelope and softly pinches fading notes.
        const float headroom=1.f/(1.f+1.9f*supplySag+.12f*env);
        const float dynamicBias=operatingBias*(.48f+.44f*supplySag);
        const float driven=std::clamp(signal*headroom,-48.f,48.f);
        struct Shape {float first,second,interstage,mix;};
        const Shape shapes[4]={{1.9f,2.8f,1.35f,.32f}, // Si aggressive hard knee
                               {1.15f,1.65f,1.00f,.46f}, // Ge rounded sustaining knee
                               {.72f,1.10f,.68f,.22f},  // LED extended swing
                               {2.35f,3.2f,1.75f,.70f}}; // BJT multi-stage gating
        const auto voiceAt=[&](int index) noexcept {
            const auto& v=shapes[static_cast<unsigned>(index)];
            const float asym=dynamicBias*(index==1?1.30f:index==3?1.5f:.95f);
            const float stage1=std::tanh(v.first*(driven+asym))-std::tanh(v.first*asym);
            const float stage2=std::tanh(v.second*(stage1*v.interstage+.65f*asym))
                             -std::tanh(v.second*.65f*asym);
            return stage1*(1.f-v.mix)+stage2*v.mix;
        };
        const float choice=std::clamp(voice,0.f,3.f);
        const int first=std::min(3,int(choice));
        const float fraction=choice-first;
        float output=voiceAt(first);
        if(first<3&&fraction>0.f)output+=(voiceAt(first+1)-output)*fraction;
        // BJT can starve harder as BIAS approaches a supply rail.
        const float bjtPinch=1.f-.28f*biasMagnitude*biasMagnitude*biasMagnitude
                                   *std::clamp(choice-2.f,0.f,1.f);
        return std::clamp(output*conduction*bjtPinch,-1.4f,1.4f);
    }
private:
    void configure(float sampleRate) noexcept {
        rateCached=std::max(8000.f,sampleRate);
        const auto pole=[this](float hz) noexcept {
            return 1.f-std::exp(-6.283185307179586f*hz/rateCached);
        };
        sagAttackPole=pole(16.f);sagReleasePole=pole(2.3f);
        gateAttackPole=pole(150.f);gateReleasePole=pole(32.f);
    }
    float rateCached=0.f,sagAttackPole=0.f,sagReleasePole=0.f,gateAttackPole=0.f,gateReleasePole=0.f;
    float supplySag=0.f,conduction=0.f;
};
