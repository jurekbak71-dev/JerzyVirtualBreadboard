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

## Windows — graficzny test audio (od 06.11.2026)

Pobierz artefakt **VBAL-Windows-GUI-x64** z [GitHub Actions](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard/actions), rozpakuj i uruchom **vbal_gui.exe** podwójnym kliknięciem. Wybierz plik PCM WAV 16/24/32-bit, wybierz filtr RC i kliknij **Przetworz WAV**. Obok wejściowego WAV powinien pojawić się plik `_RC.wav`. Nie jest wymagany PowerShell ani parametr uruchomieniowy.

To przejściowe natywne GUI Win32 do testu toru audio. Docelowy edytor breadboarda WebView2 oraz interaktywna symulacja SPICE pozostają osobnymi etapami.

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

## Generowanie netlisty z audio (prototyp SPICE)

```powershell
.\build\Release\vbal_spice.exe short_input.wav rc_from_audio.cir 10000,1e-8 0
```

Indeks kanału zaczyna się od zera. Generator akceptuje maksymalnie 4096 ramek WAV i zapisuje źródło PWL oraz obwód RC do `rc_from_audio.cir`. To **generator netlisty**, a nie wykonywanie solvera. Po instalacji ngspice netlistę należy zweryfikować osobno; pomiary `rc_output.dat` nie zostały jeszcze przetestowane. Dłuższe nagrania będą wymagały strumieniowania lub modelu źródła zewnętrznego.

## Dokumentacja
[Plan i decyzje w Notion](https://app.notion.com/p/4105fed1901b810198f7f7c008eedbf9)

## Bezpieczeństwo
Wyniki symulacji nie są gwarancją bezpieczeństwa fizycznego obwodu. Przed montażem weryfikuj zasilanie, wartości graniczne, polaryzację, pinouty i dokumentację producenta.
