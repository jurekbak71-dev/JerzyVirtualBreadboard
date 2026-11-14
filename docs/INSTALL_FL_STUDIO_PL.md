# Instalacja w FL Studio — po wydaniu zweryfikowanej paczki Windows

**Obecnie nie ma zatwierdzonej paczki instalacyjnej Windows.** Linuxowy plik `.so` nie jest wersją dla FL Studio na Windows. Nie pobieraj go jako instalatora.

Gdy będzie dostępne sprawdzone wydanie Windows x64:

1. Pobierz paczkę Windows x64 z wydania projektu i rozpakuj ją.
2. Skopiuj cały folder `Microphonic Fuzz.vst3` do `C:\Program Files\Common Files\VST3` (Windows może poprosić o uprawnienia administratora).
3. Uruchom FL Studio. Otwórz **Options → Manage plugins** i wybierz **Find installed plugins**.
4. Dodaj **Microphonic Fuzz** jako efekt na kanale miksera. Zacznij od niskiego poziomu wyjściowego.
5. A oznacza własny IR; przycisk **LOAD** wczytuje plik WAV. B dodaje wbudowaną syntetyczną odpowiedź małego pomieszczenia. Wtyczka nie wymaga fizycznego mikrofonu.

Nie instalujesz Pythona, CMake, Visual Studio ani Blendera i nie uruchamiasz terminala. Testy tej procedury w FL Studio muszą poprzedzić opublikowanie gotowego wydania.
