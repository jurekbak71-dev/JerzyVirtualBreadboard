# Virtual Breadboard Audio Lab

Publiczny projekt badawczo-rozwojowy: środowisko Windows x64 do symulacji rzeczywistych obwodów elektronicznych i przetwarzania audio.

## Status
Etap 1: referencyjny renderer WAV → filtr RC → WAV (C++20). **To nie jest jeszcze ngspice, WebView2 ani gotowy breadboard.**

## Priorytety
1. Wejście i wyjście audio (plik, później urządzenie).
2. Silnik SPICE i zgodność z referencyjnymi pomiarami.
3. Biblioteka rzeczywistych modeli komponentów.
4. Płytka stykowa, połączenia i walidacja.
5. Schemat fizyczny oraz BOM.
6. **Ostatni etap:** generowanie DSP/VST3/Raspberry Pi.

## Uruchomienie Windows
W Developer PowerShell for Visual Studio 2022:

```powershell
cmake -S native -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Aby przetworzyć własny plik PCM WAV 16/24/32-bit:

```powershell
.\build\Release\vbal_rc.exe input.wav output.wav 10000,1e-8
```

Kompilacja C++ i testy były lokalnie pozytywne na Linux/GCC. Test Windows CI wymaga sprawdzenia po publikacji.

## Dokumentacja
[Plan i decyzje w Notion](https://app.notion.com/p/4105fed1901b810198f7f7c008eedbf9)

## Bezpieczeństwo
Wyniki symulacji nie są gwarancją bezpieczeństwa fizycznego obwodu. Przed montażem weryfikuj zasilanie, wartości graniczne, polaryzację, pinouty i dokumentację producenta.
