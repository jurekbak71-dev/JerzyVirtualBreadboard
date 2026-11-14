# Microphonic Fuzz — rzeczywiste wyniki, 2026-11-14

## WYKONANO

- Załącznik `MicrophonicFuzz_DLA_CODEX.zip` rzeczywiście odczytano; oryginalne54 pliki zaimportowano z weryfikacją SHA256/CRC. Inwentarz i ograniczenia prototypów: [GUI_ASSET_INVENTORY.md](GUI_ASSET_INVENTORY.md).
- Blender 4.3.2 był zainstalowany. Wykonano rzeczywiste rendery CPU Cycles: rack, osobne panele/ramki/metal, pokrętła, mikrofon, lampa, przyciski/przełączniki/LED/VU, etykiety/skale oraz materiał/światło/refleksy/emisja/cienie. Początkowy błąd brakującego denoisera rozwiązano przez render bez denoisera, a nie symulowanie wyników.325 PNG poprawnie zdekodowano; siedem atlasów indeksów ma po128 różnych, niepustych klatek. Korpusy i refleksy pozostają nieruchome, obraca się niezależny indeks.
- Edytor JUCE zachowuje układ głównego racka z referencji.25 pokręteł,4 przełączniki i3 selektory są podłączone do32 parametrów. Przyciski nagłówka zapisują/wczytują/kopiują stan, cofają/ponawiają i przełączają źródło IR. Dostępne są nazwy dostępności i obsługa klawiatury JUCE.
- Prawdziwy sygnał steruje FFT/przebiegiem, niezależnymi miernikami L/R i RMS VU. Rysowany IR pochodzi z przygotowanego rzeczywistego jądra. Demonstracyjne grafiki waveform/spectrum z ZIP nie są używane jako analizatory.
- DSP: asymetryczna saturacja z pamięcią/envelope, bias/tone/gate, filtrowanie/emfaza, Virtual Mic z odległością0,1–4 m i ograniczonym opóźnieniem ułamkowym, modulacja, limiter/bypass, wygładzanie i zabezpieczenia wartości skończonych. Nazwy Si/Ge/LED/BJT oznaczają programowe voicingi, nie potwierdzoną emulację schematu.
- Jeden logiczny moduł konwolucji stereo pracuje **w sprzężeniu**, z jawnym opóźnieniem128 próbek w tej gałęzi i50 ms przejściem między przygotowanymi stanami. A=własny IR; B=rzeczywisty nowy syntetyczny IR fabryczny+własny IR połączone w jedno jądro. Fabryczny WAV ma generator image-source i licencjęCC0; nie jest nagraniem studia. Własny IR do8 s jest zapisywany w presecie. DECAY dodaje obwiednięT60=0,1–10 s; całkowite wybrzmienie zależy również od źródłowego IR i sprzężenia.
- Dodano Windows x64 GitHub Actions z macierzą raster ON/OFF, budową VST3, CTest i eksperymentalnym artefaktem dopiero po pozytywnych krokach. Instrukcja dla przyszłego użytkownika końcowego: [INSTALL_FL_STUDIO_PL.md](INSTALL_FL_STUDIO_PL.md).

## ZWERYFIKOWANO

Bieżąca maszyna: Debian 13 x86_64, GCC 14.2.0, CMake 3.31.6, Ninja 1.11.1, JUCE 8.0.10 (`3af3ce009f6a02f6fa651008fffb5b41743a9fab`). Zbudowano rzeczywisty **Linuxowy** pakiet VST3:

`build/baseline/MicrophonicFuzz_artefacts/Release/VST3/Microphonic Fuzz.vst3`

Test binarny ładuje samą bibliotekę VST3 przez publiczne ABI, sprawdza fabrykę, tworzy i inicjuje efekt, konfiguruje stereo48 kHz/512, przetwarza16384próbki tonu, sprawdza skończony/ograniczony i niezerowy wynik, kończy pracę i zwalnia bibliotekę. To nie jest test FL Studio.

Podglądy są świeżymi obrazami wyrenderowanymi przez rzeczywisty edytor JUCE podczas testu z przetwarzanym tonem, nie makietami ani zrzutami z DAW:

- [Rack + rzeczywiste FFT/IR](evidence/rack-gui.png)
- [Widok VU](evidence/rack-vu.png)

Rzeczywiste skrypty instalacyjne wykonano, następnie powtórzono. Toolchain znajduje się w katalogach użytkownika poza repozytorium; pakiety Debiana pobrano z podpisanych repozytoriów i rozpakowano lokalnie. Zachowano weryfikację podpisów, sum i TLS. Pliki oryginalnej paczki pozostały niezmienione.

## TESTY

| Sprawdzenie | Wynik | Dowód |
|---|---|---|
| Release, grafiki ON | PASS9/9 | [CTest](evidence/linux-raster-ctest.txt) |
| Release, grafiki OFF | PASS8/8 | [CTest](evidence/linux-fallback-ctest.txt) |
| Faktyczne VST3 load/process/unload | PASS w obu wariantach | powyższe logi |
| Wszystkie25 pokręteł/4 przełączniki/3 selektory → APVTS | PASS | `processor_smoke` |
| Cisza/impuls/ton, trzy częstotliwości, bloki0/1/512/2048, stereo, stan, RMS, resize/paint | PASS | `processor_smoke` |
|31 niezależnych efektów parametrów + osobno B, ekstremalne wartości, NaN/Inf, automatyka | PASS | `dsp_engine`, `processor_ir_recall` |
| Długi IR odpowiada FIR z opóźnieniem128; IR nie wpływa na wyjście przy feedback=0 | PASS | `long_room_convolution` |
| IR w presecie po usunięciu WAV,48↔96 kHz, odrzucenie8 s+1, zakresy/obwiednia, undo/redo | PASS | `processor_ir_recall` |
| Równoległy import/zmiany próbkowania/zapis i odczyt stanu | PASS | `concurrent_ir_state_and_sample_rate` |
| Poprawki trzech problemów synchronizacji z niezależnego code review | wdrożono; test stresowy PASS | atomowe pola, jeden mutex dla metadanych/stanu; callback bez nowego mutexa |
| SHA256/dimensions325 PNG, oryginalne54 pliki, importer | PASS | `rendered_asset_integrity`, `asset_integrity`, `importer_tests` |
| Samodzielny FuzzEngine, ASan+UBSan, Release assertions aktywne | PASS | [log](evidence/dsp-sanitizers.txt) |
|8-sekundowy gęsty IR stereo48 kHz | ok.6,94% jednego rdzenia w tej próbie | [benchmark](evidence/room-benchmark.txt) |

Benchmark mierzy sekundę generowanego audio w tej maszynie; nie potwierdza terminów czasu rzeczywistego na komputerze użytkownika ani obciążenia przejścia dwóch stanów. Sprawdzenie kodu nie wykazało jawnych alokacji/usuwania obiektów, plikowego I/O czy blokującego mutexa w `processBlock`. Nie wykonano pełnego pomiaru wszystkich alokacji callbacku. JUCE wewnętrznie używa nieblokującego spin try-lock; nie twierdzimy, że cały stos nie wykonuje żadnej operacji blokady.

## NIEZWERYFIKOWANE

- Kompilacja MSVC/Windows x64 i wyniki GitHub Actions; sama konfiguracja workflow nie stanowi wyniku.
- FL Studio: ładowanie/rozładowanie, automatyka, zapis/ponowne otwarcie projektu, rzeczywiste resize, render offline i testy buforów.
- Ostateczna akceptacja zgodności wizualnej i fotorealizmu. Layout/kierunek referencji zachowano, lecz materiały, detale siatki mikrofonu i światło lampy są rekonstrukcją, nie oryginalnymi nieznanymi modelami3D.
- Odsłuchowe porównanie charakteru analogowego, pełny pomiar aliasingu/CPU/latencji i test długotrwały na komputerze użytkownika. Brak podstaw do deklaracji dokładnej emulacji analogowej.
- Publikacja środowiska i odtworzenie w nowym zadaniu; zapis projektu konfiguracji nie jest publikacją.

## NIEWYKONANE

- Oversampling nieliniowości i porównanie1x/2x/4x/8x.
- pluginval — nie zainstalowano go w tej maszynie.
- Produkcyjna paczka instalacyjna Windows: celowo nie oznaczono linuksowego pliku jako instalatora Windows ani nie ogłoszono wydania po samych testach Linuksa.
- Końcowe zaakceptowanie całego zlecenia jako ukończonego. Wymagane pozostają powyższe bramki i ocena rzeczywistego GUI.

## Źródła i decyzje

Dokumenty wejściowe: `START_CODEX_MICROPHONIC_FUZZ.txt` z załącznika, `docs/CODEX_IMPLEMENTATION_BRIEF.md`, rzeczywisty `reference/concept_original.png`, manifesty paczki oraz późniejsze polecenie utworzenia brakujących grafik. Inwentarze/hash: `tools/gui-assets-v04.lock.json`, `tools/gui-rendered-v1.lock.json`. Geometria: oba skrypty w `tools/blender/`; etykiety: `tools/compose_gui_layers.py`. API konwolucji: rzeczywisty kod przypiętego JUCE 8.0.10.

Późniejsze żądanie pełnego GUI rozszerzyło początkowe M0. Zachowano pierwotne cztery ID i ich zakresy. Z tego powodu endpoint OUTPUT poprawiono z graficznego+12dB na rzeczywiste+6dB. Indeks porusza się nad nieruchomym korpusem/światłem. Fabryczny IR jest jawnie syntetyczny i oryginalny.128-próbkowe opóźnienie sprzężenia oraz krótkotrwałe współistnienie dwóch stanów podczas50 ms przejścia są jawnie opisane. Te wybory nie zastępują oceny brzmienia ani zatwierdzenia finalnego wyglądu.


Reprodukcja samodzielnego testu z sanitizerami:

```sh
g++ -std=c++20 -O1 -g -DNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer tests/DspEngineTests.cpp -o /tmp/mf-sanitized-dsp
/tmp/mf-sanitized-dsp
```

W projekcie konfiguracji chmurowej zapisano `install_script` (wykonane instrukcje z `tools/setup_cloud.sh`), `start_skill` (aktywacja narzędzi, build/test, snapshoty, brak trwałego serwera i zakaz tworzenia dodatkowego worktree bez żądania użytkownika) oraz dodanie `api.github.com`. Zapis potwierdzony przez usługę; wymaga przeglądu/zapisu i publikacji w ustawieniach środowiska. Nie zastosowano go automatycznie do bieżącej sieci.
