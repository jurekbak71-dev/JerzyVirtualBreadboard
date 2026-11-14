# Jerzy Microphonic Fuzz — dynamic fuzz / OCTAFUZZ implementation notes

Date: 2026-11-14. Scope: experimental feature work, not production audio acceptance.

## Approved user requests
- Existing fuzz is perceived as distortion; target: creamier, flowing sustain, a pronounced fuzz character and controllable sputter/choke.
- OCTAFUZZ appears in existing FX panel; ON/OFF, modes DOWN (−1 octave), UP (+1 octave), BOTH, and LEVEL.
- Existing tremolo, flanger, GUI atlas, parameter IDs and preset infrastructure must remain functional.

## Implemented signal path (source on codex/microphonic-fuzz-rack-dsp)
- The incoming signal is DC-blocked; a clean-input octave tracker then splits into two branches, one per stereo channel.
- DOWN toggles polarity on successive input cycles using a Schmitt trigger. Its period doubles (subharmonic 1/2); LP smooths the edges. The gate rejects very low amplitude and reduces spurious toggles, but polyphonic chords and complex distorted signals can still mistrack.
- UP uses full-wave rectification and DC rejection, which produces an octave-above component on sine input; it is *not* a general polyphonic +12 semitone shifter.
- BOTH blends the two branches at 0.72 each. Mode, octave amount and ON/OFF are smoothed; ON defaults to 0 so existing presets do not unexpectedly activate octave.
- The generated octave enters before the nonlinear fuzz. A small post-fuzz injection protects octave audibility against completely saturated gain stages.
- The fuzz uses per-channel nonlinear cascades for Si/Ge/LED/BJT, a slow supply-headroom follower, BIAS operating-point shift, and smooth conduction gating from GATE + extreme BIAS. The physical labels are voicings, not verified circuit-level analog models.
- Final safety limit remains engaged. Existing room convolution is unchanged and remains inside the Virtual Mic feedback path.

## Controls
- octOn: 0 (OFF), 1 (ON); default 0.
- octMode: 0 DOWN, 1 UP, 2 BOTH; integer-stepped, default 0.
- octLevel: 0..1; default 0.65.
- All stored as APVTS parameters, automatable and included in project/preset state.
- New controls are placed at the top of the existing FX area: OCT, DOWN/UP/BOTH, LEVEL. Existing FX controls remain in place.

## Tests and acceptance gates
- Standalone octave test: sinusoid 220 Hz should yield strong 110-Hz in DOWN and 440-Hz in UP, both in BOTH.
- Dynamic fuzz test: four distinct software transfers, compressed response to input amplitude, BIAS-dependent output, quiet-note starvation with GATE high and loud-note preservation, exact silence on zero input.
- Full FuzzEngine: OFF and level-zero isolation, spectral components through nonlinear fuzz, stress automation, finite output ceiling.
- JUCE processor/editor: parameter registration, GUI attachment, no FX button overlap, state recall.
- Windows CI: raster ON and fallback OFF builds, compiler, CTest, actual VST3 smoke test, GUI screenshots.
- User's subjective tone target **requires a real guitar/FL Studio listening test**; static numerical validation cannot certify a creamy sound or realistic gating. Aliasing and polyphonic tracking have not been measured on real performance audio.

References:
- Source code: https://github.com/jurekbak71-dev/JerzyMicrophonicFuzz/tree/codex/microphonic-fuzz-rack-dsp
- JUCE parameter attachments: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState_1_1ComboBoxAttachment.html
- Principle of analog octave divider: https://docs.architolk.nl/subwave/oc-2
- Full-wave octave fuzz example: https://d3iogksn5943rj.cloudfront.net/static/products/woctavia/Woctavia-UserManual.pdf

No production release and no rename of existing internal VST3 identifiers are authorized by this work.
