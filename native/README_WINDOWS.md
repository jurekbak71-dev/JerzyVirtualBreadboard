# Virtual Breadboard Audio Lab — natywny rdzeń audio

## Wymagania
Windows x64, Visual Studio 2022 Community lub Build Tools z komponentem Desktop development with C++, CMake.

## Kompilacja
W Developer PowerShell for VS:

```powershell
cmake -S native -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\vbal_rc.exe input.wav output.wav 10000,1e-8
```

Zamiast `input.wav` wybierz własny plik PCM WAV 16/24/32-bit, 8–384 kHz, 1–8 kanałów. Wyjście: PCM16.

## Granice prototypu
To referencyjny model matematyczny RC, a nie symulacja ngspice. WebView2/GUI oraz ASIO i wejście live nie zostały jeszcze zaimplementowane. Kompilacja MSVC wymaga weryfikacji w GitHub Actions.
