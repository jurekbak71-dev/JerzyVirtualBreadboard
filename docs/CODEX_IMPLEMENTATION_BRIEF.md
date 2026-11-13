# CODEX IMPLEMENTATION BRIEF — Microphonic Fuzz VST3
Status: ready for engineering execution; **not** a claim that DSP/GUI is complete.
Date: 2026-11-13. Repository name remains JerzyVirtualBreadboard by user decision.

## Mission / hard requirements
Implement Windows x64 VST3 for FL Studio using existing JUCE/C++20 repository, retaining history and current plugin identity/parameter IDs. Main sound goal: as convincingly **analog** as practical, judged by listening AND measurable behavior. **Virtual Mic is primary**; no physical mic required. The room impulse response must act **within** the virtual feedback path, not as an output-only reverb. One convolution engine; IR mode A = user IR only, mode B = factory library + user IR. Do not silently reinterpret A/B as two simultaneous convolution engines. No invented hardware schematic or claims of exact original pedal emulation.

## Verified baseline (inspect again before editing)
- CMakeLists.txt fetches JUCE 8.0.10, defines VST3 and conditional raster filmstrip embed.
- Source/PluginProcessor.cpp: four APVTS float parameters gain, feedback, mix, output; stereo per-channel one-sample tanh loop; no IR, oversampling, limiter, virtual mic or tests.
- Source/PluginEditor.cpp: four knobs and an optional raster LookAndFeel. PNG assets are NOT in GitHub. The v0.3 asset ZIP exists as conversation artifact only; do not pretend the build has it.
- assets/gui/vector/*.svg are earlier placeholders, NOT the approved photorealistic GUI.
- .github/workflows/import-gui-assets.yml imports v0.1 split archives, NOT v0.3. Do not confuse versions.
- Previous project is preserved in archive/virtual-breadboard-2026-11-13.

## Engineering architecture (proposal, validate against measurements)
Signal input -> DC blocker / impedance-inspired input filter -> asymmetric, stateful virtual-analog gain stages -> bounded output -> wet/dry/output. Feedback branch: nonlinear stage output -> **Virtual Mic** pickup/response -> room IR convolution -> resonance/feedback filter -> explicit stable delay -> saturating feedback gain -> sum at fuzz input. Use one shared room IR convolution instance per stereo processing path as appropriate; avoid creating an unrelated second output reverb. If convolution buffering introduces extra latency, measure and document it. Define feedback topology and compensation before coding. Avoid recursion without explicit delay and safe bounded state. Document that static IR does not simulate microphone motion; motion needs a separate, bounded, time-varying pickup/delay/filter model.

Nonlinear analog character: model input loading, frequency-dependent pre/post emphasis, asymmetrical transfer, bias/sag/envelope-memory, soft limiting and DC management; make every feature switchable for A/B measurements. Start with a simple, stable model and improve against measurable targets. Do NOT claim component-accurate circuit modeling without a real schematic and reference recordings. Consider WDF/zero-delay feedback methods after establishing testable reference behavior, not as a marketing label.

Use juce::dsp::Oversampling for nonlinear stages (evaluate 2x/4x/8x against aliasing and CPU). Its factor argument is log2 factor. Prepare/init outside audio callback; process blocks without allocation. Handle oversampling latency and dry/wet alignment. Keep room IR at base rate unless topology and measured quality justify otherwise. Parameter smoothing for gains, mix, feedback and bias. Do not allocate, lock, log, load files, perform ValueTree copyState/replaceState or construct convolution engines in processBlock. Handle zero-sized buffers, mono/stereo layout policy, bypass, sample rate/block changes, denormals, NaN/Inf recovery, preset recall and IR swap.

IR handling: juce::dsp::Convolution supports loading IR from memory/audio file, background preprocessing, trim/resample, and latency options. Investigate exact thread/ownership semantics of the pinned JUCE tag. IR switching must be click-free and safe; design an explicit staging/crossfade strategy and memory budget, without falsely promising a single engine can crossfade two independent IR states without overlap. Clarify what 'one engine' means: one logical convolution module, possibly two temporary convolution states during a transition if essential; request approval if this materially changes the interpretation. Avoid loading copyrighted factory IR without license. User IR persistence: document path versus embedded bytes, DAW portability and state size. Define maximum IR duration/size as an explicit configurable policy; no arbitrary silent truncation.

## Ordered Codex milestones
**M0 — make baseline buildable:** inspect source, JUCE 8.0.10 API and CMake; fix compilation errors; add GitHub Actions Windows MSVC x64 configure/build; compile VST3; create CTest unit harness and smoke test. Preserve existing IDs gain/feedback/mix/output. Report binary path, versions, compiler, logs. No fake passing tests.

**M1 — real-time safe DSP core:** extract DSP from AudioProcessor into testable classes; add parameter smoothing, stereo/mono policy, reset, DC blocker, safety limiter, finite-value guards, output meter. Unit tests for silence, impulse, sustained tone, extreme parameters, changing sample rates, blocks of 0/1/512/2048 samples, NaN/Inf and stability.

**M2 — analog nonlinear engine:** add controllable asymmetry, bias, sag/envelope dynamics, nonlinear frequency response; oversampling with measured aliasing and CPU/latency. Compare 1x/2x/4x/8x. Record plots and repeatable WAVs. Ensure analog-style coloration is not just random noise.

**M3 — Virtual Mic + feedback:** add explicit mic pickup transfer and position-related filter/delay; integrate convolution *in feedback*; feedback limiting and stable delay; verify zero input behavior and no runaway; verify wet path actually changes when IR changes. Expose controllable mic distance, resonance, feedback. No mandatory physical input.

**M4 — IR modes:** A/B selection with one logical IR module; import WAV user IR, optional licensed factory IR, safe resampling, asynchronous preparation and click-free transition; verify reload and preset restore. No fabricated factory library.

**M5 — approved photorealistic GUI:** only use original reference concept and approved derivative assets; separate static panel from moving controls, labels and shadows. Avoid rejected v0.2 art and SVG placeholders. Existing v0.3 ZIP is NOT in repo; require actual import and checksum validation. Implement atlas metadata (frame count, frame width/height, orientation, knob mapping), robust fallback and image bounds validation; decouple image loading from audio thread. Implement LEDs, switches, VU meter and keyboard/accessibility paths. Existing code hardcodes 1536x768 and 96px frames; validate against actual asset files before enabling. Keep graphics consistent with reference, no new design without approval.

**M6 — DAW validation and release gate:** FL Studio load/unload, automation, preset save/restore, project reopen, rapid GUI resize, 44.1/48/96kHz, 64/128/512/1024 buffer, offline render, stress IR swaps, pluginval (if available), CPU/latency/aliasing reports, no crashes and no clicks in transitions. Windows x64 VST3 artifact only after verified build and tests.

## Acceptance criteria
- Builds reproducibly in clean Windows runner; no warnings treated as silently passed tests.
- Measured real-time safety (no dynamic allocations/locks in callback in intended steady state); state reset and automation safe.
- Explicit test demonstrates that the convolution IR participates in feedback path, not merely post-processing.
- Feedback never emits NaN/Inf; extreme settings bounded; limiter safety does not hide persistent instability.
- Audio artifacts and listening comparison establish audible analog character; oversampling lowers measurable aliasing under comparable conditions. Do not claim perfect analog equivalence.
- GUI visual comparison with original approved reference; complete asset inventory, each control's states/frames, and tested interaction in plugin.
- User IR A and B behavior match requirements; license provenance for any bundled IR.
- Plugin load and automation verified in FL Studio; actual logs/screenshots or test recordings attached.
- Record each milestone in this file or separate dated evidence with PASS/FAIL/NOT RUN.

## Known risks / existing code audit targets
- Check JUCE 8 API compatibility for Slider property Var pointer and ImageFileFormat loadFrom signature; do not assume existing editor compiles.
- Check juce_add_binary_data generated resource identifiers and required target include/link; compile both MF_GUI_ASSETS=0 and =1.
- Check CMake platform guards: Windows x64 is intended; Linux CI can run headless DSP unit tests only if supported.
- Existing per-sample tanh feedback is not room/mic model. Existing 'feedback' parameter 0..0.85 does not imply stable full convolution loop.
- Static global LookAndFeel may have editor lifetime/thread concerns; prefer owned instance with correct destruction order.
- GUI v0.3 extraction/rotation may rotate highlights; asset quality remains unverified for production.
- GUI import workflow currently uses v0.1 split ZIP and writes under assets/gui; needs versioned, validated import for v0.3. Avoid committing duplicate ZIP and extracted files to main if repo growth becomes an issue.

## References checked 2026-11-13
- JUCE convolution API: https://docs.juce.com/master/classjuce_1_1dsp_1_1Convolution.html
- JUCE oversampling API: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html
- JUCE APVTS source: https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.h
- Virtual analog WDF background: De Sanctis & Sarti, IEEE TASLP 2010, https://sarti.faculty.polimi.it/Augusto_Sarti/CV_and_publications_files/2010_IEEE_TrASLP_virtual_analog_modeling_WD.pdf
- Current repo code and current Notion project decision record. Prefer exact pinned JUCE tag when implementing, not 'master' docs alone.

## First Codex instruction
Read this brief and actual repository tree. Execute M0 only, with tests and evidence. Do not rewrite the whole plugin, silently delete approved requirements, substitute placeholder DSP or claim later milestones are complete. Open a PR with reproducible Windows build and honest PASS/FAIL/NOT RUN results.