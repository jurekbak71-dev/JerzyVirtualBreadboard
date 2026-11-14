# GUI asset inventory — 2026-11-14

Source: user upload MicrophonicFuzz_DLA_CODEX.zip, not the old v0.1 split import.
Archive SHA256: `544df0c070cda653ebd199efd951169f18fabb5cc971a3d3a7174f49eae607f7`.
All 54 files imported unchanged under assets/gui/production; original manifests retained. Per-file sizes, dimensions, alpha extrema and SHA256: tools/gui-assets-v04.lock.json. Importer verifies archive hash, ZIP inventory and every file before writing; repeat import preserves identical output and rejects changed files.

## Visual comparison and production gate

The approved reference is a 1672×941 composite rack and parts board. The baseline editor contains only four attached controls and is NOT the complete approved rack. The seven knob atlases are cropped/rotated derivatives: highlights, tick markings and shadows rotate with the knob rather than staying fixed. The panel is a blurred crop, not a full rack background. v0.4 animated assets are procedural 2.5D approximations, explicitly not certified final 3D renders. All are AVAILABLE as prototypes; production quality and host interaction are UNVERIFIED. No substitute art has been generated.

MISSING: separate full rack background, independent fixed lighting/shadows, complete module panels, separate labels/scales/icons with layout/interaction metadata, finalized mic and tube renders, certified production control states. Typography and original layout are visible only in the composite, which cannot serve as an interactive production panel. Production photorealistic GUI is BLOCKED on adequate derivative assets and visual acceptance.

Waveform/spectrum atlases are illustrative only; they must not be connected to a timer and described as live analyzers. Actual signal capture/FFT rendering is NOT IMPLEMENTED. VU atlas combines frame and needle; dB calibration, ballistics and real signal wiring are NOT IMPLEMENTED. Selector, toggle, LED, mic, lamp, fader states exist but their corresponding DSP/parameters do not exist in M0; do not animate dummy states.

Atlas contract: row-major, frame = round(clamp(normalized,0,1) × (frames-1)); source x=(frame%columns)×frameWidth, y=(frame/columns)×frameHeight. Bounds must match declared rows/columns before drawing. Four existing controls retain host automation and keyboard access through JUCE/APVTS. Full accessibility and interaction of the approved rack remain NOT RUN.

Dimensions/alpha and distinct frame counts below were decoded with Pillow 11.3.0. Blank unused cells are excluded; unique pixels do not establish correct motion or photorealism. Hash integrity is not artistic approval.

| File | Status | Atlas pixels | Mode / alpha min,max | Frames / distinct |
|---|---|---|---|---|
| `animations/backlight_32.png` | AVAILABLE / prototype | 1920×320 | RGBA [0, 10] | 32 / 27 |
| `animations/button_amber_16.png` | AVAILABLE / prototype | 1792×72 | RGBA [0, 255] | 16 / 16 |
| `animations/button_green_16.png` | AVAILABLE / prototype | 1792×72 | RGBA [0, 255] | 16 / 16 |
| `animations/button_neutral_16.png` | AVAILABLE / prototype | 1792×72 | RGBA [0, 255] | 16 / 16 |
| `animations/button_red_16.png` | AVAILABLE / prototype | 1792×72 | RGBA [0, 255] | 16 / 16 |
| `animations/fader_128.png` | AVAILABLE / prototype | 1280×1600 | RGBA [0, 255] | 128 / 128 |
| `animations/led_amber_64.png` | AVAILABLE / prototype | 1280×320 | RGBA [0, 255] | 64 / 64 |
| `animations/led_blue_64.png` | AVAILABLE / prototype | 1280×320 | RGBA [0, 255] | 64 / 64 |
| `animations/led_green_64.png` | AVAILABLE / prototype | 1280×320 | RGBA [0, 255] | 64 / 64 |
| `animations/led_red_64.png` | AVAILABLE / prototype | 1280×320 | RGBA [0, 255] | 64 / 64 |
| `animations/led_white_64.png` | AVAILABLE / prototype | 1280×320 | RGBA [0, 255] | 64 / 64 |
| `animations/level_meter_33.png` | AVAILABLE / prototype | 672×444 | RGBA [0, 255] | 33 / 17 |
| `animations/selector_active_0.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_active_1.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_bypass_0.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_bypass_1.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_chaos_0.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_chaos_1.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_limiter_0.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_limiter_1.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_room_a_0.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_room_a_1.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_room_b_0.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_room_b_1.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_scream_0.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_scream_1.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_sustain_0.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/selector_sustain_1.png` | AVAILABLE / prototype | 148×58 | RGBA [0, 255] | 1 / 1 |
| `animations/spectrum_64.png` | AVAILABLE / prototype | 2048×800 | RGBA [0, 255] | 64 / 64 |
| `animations/toggle_2pos_16.png` | AVAILABLE / prototype | 1536×96 | RGBA [0, 255] | 16 / 2 |
| `animations/toggle_3pos_33.png` | AVAILABLE / prototype | 1536×288 | RGBA [0, 255] | 33 / 33 |
| `animations/tube_glow_64.png` | AVAILABLE / prototype | 960×1440 | RGBA [0, 255] | 64 / 64 |
| `animations/virtual_mic_movement_64.png` | AVAILABLE / prototype | 1280×1536 | RGBA [0, 255] | 64 / 58 |
| `animations/vu_needle_128.png` | AVAILABLE / prototype | 2048×2176 | RGBA [0, 255] | 128 / 128 |
| `animations/waveform_64.png` | AVAILABLE / prototype | 2048×800 | RGBA [0, 255] | 64 / 64 |
| `knobs/black_anodized_128.png` | AVAILABLE / prototype | 1536×768 | RGBA [0, 255] | 128 / 128 |
| `knobs/black_small_128.png` | AVAILABLE / prototype | 1536×768 | RGBA [0, 255] | 128 / 128 |
| `knobs/brass_small_128.png` | AVAILABLE / prototype | 1536×768 | RGBA [0, 255] | 128 / 128 |
| `knobs/chrome_small_128.png` | AVAILABLE / prototype | 1536×768 | RGBA [0, 255] | 128 / 128 |
| `knobs/steel_bright_128.png` | AVAILABLE / prototype | 1536×768 | RGBA [0, 255] | 128 / 128 |
| `knobs/steel_large_128.png` | AVAILABLE / prototype | 1536×768 | RGBA [0, 255] | 128 / 128 |
| `knobs/steel_warm_128.png` | AVAILABLE / prototype | 1536×768 | RGBA [0, 255] | 128 / 128 |
| `panels/brushed_dark_panel.png` | AVAILABLE / prototype | 600×348 | RGB None | static / n/a |
| `reference/concept_original.png` | AVAILABLE / prototype | 1672×941 | RGBA [255, 255] | static / n/a |
| `switches/led_amber.png` | AVAILABLE / prototype | 72×72 | RGBA [0, 255] | static / n/a |
| `switches/led_red.png` | AVAILABLE / prototype | 72×72 | RGBA [0, 255] | static / n/a |
| `switches/toggle_0.png` | AVAILABLE / prototype | 76×96 | RGBA [0, 255] | static / n/a |
| `switches/toggle_1.png` | AVAILABLE / prototype | 76×96 | RGBA [0, 255] | static / n/a |
| `switches/toggle_2.png` | AVAILABLE / prototype | 76×96 | RGBA [0, 255] | static / n/a |

## Update: implemented 3D reconstruction, 2026-11-14

The missing-item assessment above describes the **uploaded v0.4 pack**, not the new assets. In response to the subsequent explicit request to create the missing art, Blender 4.3.2 was found and genuinely used with CPU Cycles. No ImageGen replacement concept was created.

New `assets/gui/rendered-v1` supplies the rack chassis/wood cheeks, twelve independent panels/frames and their named metal/screw meshes in the source scene, seven machined knob bodies, seven separate 128-frame index atlases, five LED colors, two/three-position toggles, button/selector states, rounded microphone capsule/chamber, rounded glass tube/cage/filament, VU face/needle, and a separate label/scale overlay. Material, direct/indirect light, reflection, emission and true shadow-catcher passes are exported separately. Beauty images cache the fixed lighting; only the index geometry moves over each knob body, so highlights and scales do not rotate with knob automation.

`tools/gui-rendered-v1.lock.json` records **325 fully decoded PNGs** with exact dimensions and SHA256; all seven index atlases have 128 nonempty, distinct frames. Original prototype files remain separately versioned and unchanged. Blender scene scripts, rather than flattened artwork, are the reproducible geometry sources; saved `.blend` files are working scene snapshots.

JUCE owns image/style lifetimes, embeds actual files through BinaryData, validates integrity before raster builds, maps frames 0–127, exposes keyboard/accessibility names, and connects controls through APVTS. Real audio drives spectrum/scope, independent stereo peak meters, RMS VU and mic movement. Original illustrative waveform/spectrum atlases are not used.

**Remaining visual limitation:** this is a reference-based reconstruction, not a certified pixel-identical render of unknown original 3D source geometry. The new metal finish, grille detail and tube glow differ from the composite reference. The physical layout and visual direction are preserved; final aesthetic acceptance and real Windows/FL Studio interaction remain UNVERIFIED. Do not call it the approved final production GUI on the basis of file counts or checksums.
