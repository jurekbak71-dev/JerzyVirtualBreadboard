# Uruchomienie Microphonic Fuzz w FL Studio

Dostępna jest **eksperymentalna paczka Windows x64**, która przeszła budowę i testy GitHub Actions. Nie została jeszcze sprawdzona w FL Studio.

1. Otwórz [zielony przebieg GitHub Actions](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard/actions/runs/43584723784). W sekcji **Artifacts** pobierz **MicrophonicFuzz-M0-Windows-x64-experimental** (14,4 MB). GitHub może wymagać zalogowania.
2. Rozpakuj pobrany ZIP.
3. Skopiuj cały folder `Microphonic Fuzz.vst3` do `C:\Program Files\Common Files\VST3`. Windows może poprosić o uprawnienia administratora.
4. Uruchom FL Studio. Otwórz **Options → Manage plugins** i wybierz **Find installed plugins**.
5. Dodaj **Microphonic Fuzz** jako efekt na kanale miksera. Zacznij od niskiego poziomu wyjściowego.
6. **LOAD** wczytuje własny IR WAV. A używa własnego IR; B dodaje wbudowaną syntetyczną odpowiedź małego pomieszczenia. Fizyczny mikrofon nie jest potrzebny.

Nie instalujesz Pythona, CMake, Visual Studio ani Blendera i nie uruchamiasz terminala. Przed uznaniem wersji za produkcyjną pozostają faktyczne testy ładowania, automatyki i zapisu projektów w FL Studio.
