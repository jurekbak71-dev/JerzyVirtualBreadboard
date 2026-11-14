# Jerzy Microphonic Fuzz VST3

JUCE 8.0.10 / C++20 audio effect targeting Windows x64 and FL Studio. The repository name and original `gain`, `feedback`, `mix`, `output` parameter IDs are retained. The old breadboard project remains on `archive/virtual-breadboard-2026-11-13`.

The reference-based rack now has separate Blender/Cycles geometry, control bodies, 128-frame moving indices, labels, lighting/reflection/shadow passes, and an interactive JUCE editor. This is a reconstruction of the supplied design, not a new visual concept or a claim of pixel-identical final art. See [asset inventory](docs/GUI_ASSET_INVENTORY.md) and the dated implementation evidence for validation and remaining release gates.

## Development

Windows: CMake 3.22+, Python 3, Visual Studio C++ x64 toolchain; JUCE is fetched at tag `8.0.10`.

```powershell
cmake -S . -B build -A x64 -DMF_GUI_ASSETS=ON -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure --no-tests=error
```

`.github/workflows/windows-build.yml` runs both raster ON and fallback OFF builds. Uploaded artifacts are explicitly experimental; workflow configuration alone does not establish a successful Windows build.

For the selected Debian 13 cloud machine, `bash tools/setup_cloud.sh` installs a user-owned, signature-verified development toolchain without administrative system changes. Then:

```sh
. /workspace/mf-toolchain/activate.sh
xvfb-run -a ctest --test-dir build/baseline --output-on-failure --no-tests=error
```

This cloud task is already isolated; use its existing checkout without creating a Git worktree. GUI work requires a display or Xvfb. `MF_SCREENSHOT_DIR=/absolute/output/path` makes the processor smoke test save fresh JUCE-rendered spectrum and VU snapshots.

## Assets

The original upload is imported unchanged under `assets/gui/production` (54 files). `tools/import_gui_assets.py <archive>` accepts only the supplied archive checksum; `--verify` checks the imported files. Prototype waveform/spectrum animations are never used as signal analyzers.

New geometry and artwork live in `assets/gui/rendered-v1`. `tools/audit_gui_assets.py` verifies the versioned file inventory and hashes. Blender 4.3.2 CPU Cycles renders are reproducible with `tools/blender/render_assets.py` and `render_controls.py`; `tools/compose_gui_layers.py` creates the text/scale overlay. Authoring/auditing pixels requires Pillow; playback and installation do not require Python or Blender. `render_assets.py -- --layers` exports separate material, direct/indirect light, reflection, emission and genuine shadow-catcher passes. Shadow-catcher passes contain multiplicative lighting, not ready-to-paste alpha shadows.

## New analog-inspired FUZZ + OCTAFUZZ stage (experimental)

The dynamic fuzz stage includes two nonlinear stages per channel, slower envelope
headroom sag, a BIAS-dependent nonlinear operating point, and a controllable
starvation/gating envelope. Si, Ge, LED and BJT represent different software
voicings. In extreme BIAS/GATE settings notes can pinch off; lower GATE with
moderate BIAS gives a smoother compressed sustain. These are software
approximations, not a physically calibrated analog circuit. Auditioning is still
required to assess the user's desired creamy, flowing character.

The dedicated **OCTAFUZZ** strip is inside the existing FX module without
removing tremolo or flanger. OCT ON/OFF, a three-way **DOWN / UP / BOTH** switch,
and **LEVEL** are independent automatable parameters saved in VST3 presets.
DOWN (−1) is a Schmitt-controlled polarity flip of successive input periods
with low-pass rounding, UP (+1) uses full-wave rectification and DC rejection;
BOTH mixes the two signals. Both octave signals originate from the *clean*
guitar input and feed the fuzz stage, with a small additional post-stage
injection to retain the octave component at very high gain. Switching and level
are smoothed. When OFF (default) or LEVEL is zero there is no octave contribution.
The divider is intended for single-note guitar: chords, noisy pickups, bends
and very low signal levels can cause mistracking, like a simple analog divider.
This is not a transparent polyphonic pitch shifter. No fresh claim of
zero aliasing, analog measurement fidelity or FL Studio playback is made.

## Signal and controls

The four software fuzz voicings (Si/Ge/LED/BJT) have distinct asymmetric transfer curves; BIAS changes positive/negative headroom. The feedback loop tracks its own signal level and reduces injection as its envelope rises. These models use envelope-dependent drive, DC management, tone/gating and smoothing. Si/Ge/LED/BJT names identify software voicings; no component-accurate circuit or exact pedal emulation is claimed. No calibrated analog, aliasing, listening or FL Studio validation is claimed for these new changes. Filter RES controls currently adjust filter emphasis, rather than a calibrated resonant hardware circuit.

Virtual Mic models 0.1–4 m propagation, bounded motion with fractional delay, position-dependent filtering and pickup. Room convolution is **inside feedback**. One logical stereo room module processes 128-sample quanta; the feedback path therefore has an explicit additional 128-sample delay (2.67 ms at 48 kHz), not a second output reverb. USER selects only the uploaded WAV; FACTORY combines one of five original synthetic IRs (Booth, Small Room, Studio, Plate or Hall) with an optional user WAV in one normalized kernel. The included five factory WAV files are original, explicitly synthetic geometry/tap/decay models, not measured room recordings; see `assets/ir/LICENSE.txt` and `tests/GenerateFactoryIR.py`.

The DECAY control applies an additional 0.1–10 s exponential T60 envelope when preparing the kernel; the source IR still determines its overall acoustic decay. The original output range is retained (0–2, at most +6 dB), and the endpoint label reflects that range rather than the reference’s +12 dB. A final ±0.995 sample-safety ceiling remains engaged with BYPASS or limiter OFF; it is not a true-peak or oversampling limiter.

User IR policy: mono/stereo, 8–384 kHz source, at most eight seconds; oversized input is rejected, never silently truncated. The mono-downmixed coefficients and their sample rate are embedded in presets, so recall does not require the source WAV. Prepared convolver states switch with a 50 ms transition; temporary old/new states overlap within the single logical module. Preparation and destruction occur outside the callback. JUCE internally uses a nonblocking spin try-lock; no claim of absolutely zero internal lock operations is made.

The waveform and 512-point Hann-window FFT show actual output samples. The analyzer is a short-window display, not a precision spectrum measurement. Double-click the analyzer, or use the settings menu, to select the RMS-driven VU view (300 ms smoothing; 0 VU = −18 dBFS). L/R meters show independent actual peak levels. Header controls select IR source, save/load/copy plugin state and undo/redo parameter edits.

## Release gate

A successful Linux VST3 build does not validate Windows or FL Studio. Windows x64 raster/fallback builds and tests passed in [run43584723784](https://github.com/jurekbak71-dev/JerzyMicrophonicFuzz/actions/runs/43584723784), which supplies an experimental Windows artifact. FL Studio loading/automation/project recall, pluginval where available, visual acceptance, listening comparisons, nonlinear oversampling and aliasing measurements remain required before a production installer is published. No production Windows installer is claimed or supplied by this source checkout.

After a verified Windows package exists, the end-user instructions are [here](docs/INSTALL_FL_STUDIO_PL.md). End users do not need a compiler, CMake, Python or a terminal.
