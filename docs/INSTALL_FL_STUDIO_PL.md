# Uruchomienie Jerzy Microphonic Fuzz w FL Studio

Dostępna jest **historyczna eksperymentalna paczka Microphonic Fuzz Windows x64** z wcześniejszej kompilacji GitHub Actions. Nowa nazwa Jerzy Microphonic Fuzz oraz pięć IR nie są jeszcze potwierdzone jako działający artefakt do czasu osobnej zielonej kompilacji Windows. Nie wykonano testów w FL Studio.

1. Otwórz [zielony przebieg GitHub Actions](https://github.com/jurekbak71-dev/JerzyMicrophonicFuzz/actions/runs/43584723784). W sekcji **Artifacts** pobierz **MicrophonicFuzz-M0-Windows-x64-experimental** (14,4 MB). GitHub może wymagać zalogowania.
2. Rozpakuj pobrany ZIP.
3. Skopiuj cały folder `Microphonic Fuzz.vst3` do `C:\Program Files\Common Files\VST3`. Windows może poprosić o uprawnienia administratora.
4. Uruchom FL Studio. Otwórz **Options → Manage plugins** i wybierz **Find installed plugins**.
5. Dodaj **Microphonic Fuzz** jako efekt na kanale miksera. Zacznij od niskiego poziomu wyjściowego.
6. W historycznej paczce **LOAD** wczytuje WAV, a A/B wybiera USER albo pojedynczy IR syntetyczny. Nowsza gałąź `codex/microphonic-fuzz-rack-dsp` ma USER/FACTORY i pięć IR: Booth, Small Room, Studio, Plate, Hall. 
7. Nowy moduł **OCTAFUZZ** w sekcji FX ma ON/OFF, wybór DOWN (−1), UP (+1), BOTH i LEVEL. Przy dużych ustawieniach BIAS i GATE fuzz może się wyraźnie „zatykać”. Brzmienie wymagające odsłuchu nie jest jeszcze potwierdzone w FL Studio.

**Aktualną eksperymentalną paczkę** pobieraj z najnowszego zielonego przebiegu [GitHub Actions Windows](https://github.com/jurekbak71-dev/JerzyMicrophonicFuzz/actions/workflows/windows-build.yml), artefakt `JerzyMicrophonicFuzz-Windows-x64-experimental`. Nie zakładaj, że starszy przebieg zawiera OCTAFUZZ; sprawdzaj SHA commita i wynik testów.

Nie instalujesz Pythona, CMake, Visual Studio ani Blendera i nie uruchamiasz terminala. Przed uznaniem wersji za produkcyjną pozostają faktyczne testy ładowania, automatyki i zapisu projektów w FL Studio.
