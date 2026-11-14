#pragma once
#include <algorithm>
#include <cmath>

// Lightweight mono, analog-style octave generator for the input of the fuzz.
// DOWN: Schmitt-triggered polarity flip every INPUT period (+/- alternating cycles).
// UP: full-wave rectification and DC rejection. This is not a polyphonic pitch shifter.
// No heap allocation, FFT, resampling buffers or delays. Each channel owns its state.
class AnalogOctave
{
public:
    void reset() noexcept { tracker = envelope = rectifiedMean = upLow = downLow = 0.0f;
                            schmittHigh = false; divideState = false; }
    struct Samples { float down=0.0f, up=0.0f; };

    Samples generate(float input, float sampleRate) noexcept
    {
        if (sampleRate != rateCached) configure(sampleRate);
        // Restrict harmonics in the input to avoid spurious suboctave triggers.
        tracker += trackerPole * (input - tracker);
        const float level = std::abs(tracker);
        envelope += (level > envelope ? attackPole : releasePole) * (level - envelope);
        const float threshold = std::max(0.0012f, 0.14f * envelope);
        if (envelope > 0.003f)
        {
            if (!schmittHigh && tracker >= threshold)
            {
                schmittHigh = true;
                divideState = !divideState; // one toggle per input period, so f/2
            }
            else if (schmittHigh && tracker <= -threshold)
                schmittHigh = false;
        }
        else schmittHigh = false;

        // Successive input cycles alternate polarity. Rounded LP tames edges.
        const float subRaw = (divideState ? 1.0f : -1.0f) * tracker;
        downLow += downPole * (subRaw - downLow);
        const float sub = downLow;

        // Full-wave rectifier: |sin(wt)| includes a component at 2*w.
        const float rectified = std::abs(tracker);
        rectifiedMean += meanPole * (rectified - rectifiedMean);
        const float doubler = 1.6f * (rectified - rectifiedMean);
        upLow += upPole * (doubler - upLow);
        const float upper = upLow;

        // Mute when the detector has no input. Both branches track while OFF.
        const float valid = std::clamp(envelope / .009f, 0.f, 1.f);
        return { std::clamp(sub * valid, -1.5f, 1.5f),
                 std::clamp(upper * valid, -1.5f, 1.5f) };
    }

    float process(float input, float sampleRate, int mode) noexcept
    {
        const auto signals = generate(input, sampleRate);
        return mode == 0 ? signals.down : mode == 1 ? signals.up
                         : (0.72f * signals.down + 0.72f * signals.up);
    }
private:
    void configure(float sampleRate) noexcept {
        rateCached = std::max(8000.0f, sampleRate);
        const auto pole = [this](float hz) noexcept {
            return 1.0f - std::exp(-6.283185307179586f * hz / rateCached);
        };
        trackerPole=pole(1500.f);attackPole=pole(85.f);releasePole=pole(18.f);
        downPole=pole(560.f);meanPole=pole(35.f);upPole=pole(5500.f);
    }
    float rateCached=0.f,trackerPole=0.f,attackPole=0.f,releasePole=0.f;
    float downPole=0.f,meanPole=0.f,upPole=0.f;
    float tracker=0.0f, envelope=0.0f, rectifiedMean=0.0f;
    float upLow=0.0f, downLow=0.0f;
    bool schmittHigh=false, divideState=false;
};
