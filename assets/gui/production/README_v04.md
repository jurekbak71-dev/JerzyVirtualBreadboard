# Microphonic Fuzz v0.4 — uzupełnienie elementów animowanych

Oparte stylistycznie na zatwierdzonym oryginalnym panelu, nie na odrzuconej wersji v0.2. Zawiera dotychczasowe zasoby v0.3 oraz nowe atlasy animacji: LED (5 barw × 64), toggle (2/3 pozycje), przyciski (4 × 16), VU (128), fader (128), level meter (33), waveform/spectrum (64), glow (32), virtual mic (64), lampę (64), stany selektorów.

**Ograniczenia:** nowo wygenerowane elementy są proceduralnymi przybliżeniami 2.5D, a nie niezależnymi fotorealistycznymi renderami 3D. Grafiki przebiegów i widma są przykładowymi animacjami, nie analizą dźwięku. Zasoby nie są zatwierdzonym końcowym GUI i nie były testowane w JUCE/FL Studio.

**Integracja:** użyj `manifest_v04.json` do odczytu liczby klatek, rozmiarów i kolumn atlasów. Klucz `animations/...` wskazuje pliki PNG. Wartości VU, LED i widma muszą pochodzić z rzeczywistych pomiarów DSP.
