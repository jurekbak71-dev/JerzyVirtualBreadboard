# Vbreadboard — nowe laboratorium efektów, PT2399 i instrumenty (08.11.2026)

Status: **działający, częściowo zweryfikowany prototyp**, nie uniwersalny fizyczny symulator wszystkich pedałów. Wszystkie efekty poniżej są **edytowalnymi obwodami**, a nie makrami DSP. Inspiracja rodziną efektów nie oznacza, że to precyzyjna kopia projektu handlowego.

## Nowe warianty (14)

| Klucz | Charakterystyka i części | Co zmienić |
| --- | --- | --- |
| `mufftone` | pasywny tone stack Big Muff-family: dwie gałęzie RC i trzy końcówki P1 | R1–R3, C1–C2, P1 |
| `muffdark` | alternatywne wartości tej samej sieci | R, C, P1 |
| `rangetreble` | germanowy PNP ze wzmacniaczem WE, ujemne zasilanie | dzielnik bazy, C1, P1 |
| `lpbboost` | booster NPN z polaryzacją DC i kondensatorami sprzęgającymi | R3, C1, P1 |
| `asymclip` | asymetryczne diody Ge/Si | wartości R, kolejność i typ diod |
| `tsfeedback` | opamp z dwiema diodami w pętli sprzężenia | R3, C2, diody, P1 |
| `rathard` | opamp z diodami krzemowymi do masy, filtr RC | R3–R6, C1, P1 |
| `distplus` | opamp i diody germanowe do masy | R3, diody, P1 |
| `bridgedt` | pasywna sieć Bridged-T | R1–R4, C1–C2, P1 |
| `tilteq` | dwie gałęzie RC mieszane potencjometrem | C1–C2, P1 |
| `cockedwah` | statyczne RLC z rezonansem; **bez wah/LFO** | L1, C1, P1 |
| `allpass` | stały przesuwnik fazy opamp RC; **bez modulowanego phasera** | R1–R3, C1 |
| `zenerdrive` | dioda Zenera 5,1 V z Si | R1, diody |
| `fuzztwo` | dwa stopnie NPN, sprzęgające C i P1 | beta Q1/Q2, R, C, P1 |

Poprzednie obwody PT2399 DIP-16, MN3007 + MN3101 DIP-8, Fuzz Face, EQ 3-pasmowy i pozostałe pozostają w bibliotece.

### Sposób sprawdzenia

`node web/test_wiring.js --export-analog <katalog>` zapisuje dokładne netlisty *wygenerowane przez aktualny edytor*, które można przetworzyć przez `vbal_render_graph`. Test `native/tests/analog_library_integration.py` uruchamia rzeczywiste ngspice dla każdego z 14 wariantów, weryfikuje 6000 ramek PCM WAV i obecność sygnału (test nie certyfikuje brzmienia, stabilności w każdej konfiguracji ani zgodności z miernikami). Dodatkowo CTest sprawdza solver MNA i modele PT2399/MN3007.

W toku testów wykryto niezbieżność obwodu RAT-family; naprawiono wprowadzeniem gładkiego nasycania w modelu źródła opamp w ngspice. Starszy format `V=max(...)` jest akceptowany przez parser dla kompatybilności. Korekta dzielnika bazy PNP w Rangemaster-family poprawiła poziom w próbce testowej z **30 do 9311 jednostek PCM** przy tym samym wejściu.

Wysokie poziomy niektórych układów (szczególnie dwa stopnie fuzz i NPN booster) mogą powodować **clipping przy eksportowaniu WAV**; należy regulować wejście i poziom wyjściowy, nie utożsamiać szczytu PCM z prawidłowym zakresem elektrycznym modelu.

### PT2399 i BBD

- PT2399: 16-pinowy komponent w fizycznym grafie MNA, sterowanie pinem 6 do DGND przez rzeczywisty rezystor; zmieniono charakterystykę R–czas na punkty z **Princeton V1.9 (2022)**. Po poprawnym zasileniu model ma przybliżony **400 ms okres wyciszenia/resetu**. Karta Princeton w jednym punkcie `3.76 kΩ` zawiera niejednoznaczny zapis „7504ms”; interpolacja/interpretacja wymaga potwierdzenia pomiarami egzemplarza układu. Model pozostaje makromodelem ADC/RAM/DAC, **nie** tranzystorową emulacją silikonu.
- MN3007/MN3101: rzeczywiste piny, osobne zasilanie, RC generatora, sterowanie 1024-stopniową pamięcią BBD w VBAL MNA. Ostatnio poprawiono obliczanie napięć wejścia względem **lokalnego GND**. Pomiarowe straty ładunku, clock feedthrough, dokładne szumy i nieliniowości MOSFET nie zostały zweryfikowane ani kompletnie odwzorowane.
- `ngspice` symuluje analogowe R/C/L/D/Q/OPAMP; PT2399 i MN3007/MN3101 obsługuje **VBAL hybrid solver**, nie nieistniejące standardowe modele SPICE scalaków producentów.

### Instrumenty, pokrętła i import

- W głównym widoku **dwa oscyloskopy**: A = sonda różnicowa MNA, B = rzeczywiste wejście do solvera. Dane pochodziły z natywnej warstwy audio; po rozbudowie bufora ślad zapisywany jest dla każdej próbki, a odczyt 256-punktowy ma krok 1/4/16/64 próbek (zakres ok. 5,3/21,3/85,3/341 ms przy 48 kHz). Testy live na sprzęcie użytkownika **NIEZWERYFIKOWANE**.
- Każdy `POT` ma rzeczywiste trzy wyprowadzenia i mechaniczne pokrętło SVG (płytka + inspektor). **Od 08.11.2026 gest, klawiatura i zmiana wartości całkowitej wysyłają aktualizacje do działającego solvera bez restartu audio**; wartości są wygładzane w MNA przez około 4 ms.
- Import strukturalny: KiCad XML i `.kicad_sch` przez oficjalny `kicad-cli sch export netlist --format kicadxml`; dodatkowo obsługiwany podzbiór netlist SPICE. Rozpoznawanie dowolnych symboli/przewodów ze zdjęć i skanowanych PDF-ów **NIEWYKONANE**. Dodano **wizualną referencję PNG/JPG/WebP/PDF** do ręcznego porównywania obwodu — samo wyświetlenie grafiki nie tworzy netlisty.

### Źródła referencyjne

- [Princeton Technology — PT2399 V1.9](https://www.princeton.com.tw/en-us/Products/Multi-Media-Audio/Echo-Processor/EP-PT2399)
- [KiCad CLI — netlisty](https://docs.kicad.org/master/en/cli/cli.html)
- [General Guitar Gadgets — schematics](https://generalguitargadgets.com/how-to-build-it/technical-help/schematics/)
- [Parasit Studio — layouts](https://parasitstudio.com/resources/layouts/)
- [Tagboard Effects — stripboard](https://tagboardeffects.blogspot.com/) (szczegółowe layouty nie zawsze dostępne z uwagi na ograniczenia dostępu)
- [FreeStompboxes — DIY forum](https://www.freestompboxes.org/viewforum.php?f=13) (część wątków wymaga dostępu forum)
- [Wampler Pedal Course — kalkulatory filtrów](https://www.guitarpedalcourse.com/pages/free-guitar-pedal-circuit-calculators-guitarpedalcourse)
- [ngspice B sources — dokumentacja nieliniowych źródeł](https://nmg.gitlab.io/ngspice-manual/non-lineardependentsources_behavioralsources/bxxxx_nonlineardependentsource_asrc/syntaxandusage.html)

Nie kopiowano cudzych schematów ani grafik do repozytorium; należy sprawdzić licencję/zgodę przed redystrybucją oryginałów. Warianty zachowują własne, uproszczone wartości i topologie; wymagają odrębnej weryfikacji z konkretnymi publikowanymi schematami.

## Iteracja 08.11.2026 — LIVE potentiometers, wyjścia BBD, SVG z metadanymi

**WYKONANO w kodzie:** [commit źródeł `0fd801db`](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard/commit/0fd801db2895a889c73c88def80d43a510d30efe).

- Zlikwidowano restart silnika audio podczas ruchu potencjometrem. Protokół `VBAL_POT|P1|ohms|0.5` aktualizuje atomowo dwa rezystory `RP1A` i `RP1B` w istniejącym grafie. Solver MNA odczytuje cele bez blokad i wygładza wartość 4 ms, dzięki czemu nie resetuje stanu kondensatorów, PT2399/BBD ani transportu WASAPI. Pozostałe zmiany obwodu, np. dodanie przewodu, nadal przebudowują graf.
- Ergonomiczne pokrętła: średnice zwiększone w widoku fizycznym i logicznym oraz inspektorze; mechaniczny pierścień z 11 znacznikami, regulacja przeciąganiem, kołem, strzałkami, Home/End, wskazanie procentowe w trakcie ruchu. Inspektor zwiększony do 440 px wysokości, przesuwanie nóżek A/W/B przeniesione do zwijanego bloku.
- PT2399 pin 6: rezystor sterujący VCO jest teraz odczytywany z tego samego atomowego regulatora; test porównuje czas przyjścia impulsu po zmianie 2 kΩ → 9,2 kΩ z ustalonym układem 9,2 kΩ bez odtwarzania pamięci scalaka.
- MN3101 → MN3007: fizyczny oscylator sterowany rezystorem RC może zmieniać częstotliwość, a przez to czas przepływu 1024 stopni BBD, podczas audio LIVE. Rozdzielono OUT1 i OUT2 MN3007 na naprzemienne fazy ładunku na pinach 7/8 (makromodel; zewnętrzne sumowanie może ograniczyć widoczność komponentów zegarowych). Nie jest to pełny tranzystorowy model Panasonic.
- Import **graficznego SVG Vbreadboard zawierającego zapis metadanych**: wyeksportowany przez aplikację SVG zawiera JSON `vbal-project-json`, pozwalający ponownie odtworzyć części, zworki i położenia fizyczne. Dowolny obcy SVG bez danych elektrycznych jest odrzucany — system nie wymyśla połączeń.
- **NIEWYKONANE:** automatyczne i wiarygodne rozpoznawanie sieci elektrycznych z dowolnego PNG/JPG/skanowanego PDF-u/strony WWW. Podgląd referencji nie jest parserem. **NIEWYKONANE:** pełny model wszystkich wewnętrznych tranzystorów PT2399, MN3007 i MN3101; publiczne dokumentacje podają schemat blokowy i charakterystyki, nie kompletny zweryfikowany opis maski/sieci wewnętrznych.

**ZWERYFIKOWANO / TESTY:**
- [Linux CI #42501401433](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard/actions/runs/42501401433) — **SUCCESS**, CTest 7/7, reguły i przebiegi WAV/ngspice dla 14 analogowych układów, testy fizycznego BBD/PT2399, JS: brak restartu podczas ruchu gałką, aktualizacja netlisty, weryfikacja metadanych SVG.
- [Windows CI #42501401534](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard/actions/runs/42501401534) — **SUCCESS** dla tego samego SHA, build Windows x64, testy i archiwum `VBAL-Windows-SPICE-GUI-x64` (ID 13626030028; 21 998 289 bajtów). Zapis paczki do GitHub Actions potwierdzony; **nie pobierano jej ani nie odsłuchiwano na komputerze użytkownika**.
- Potrzebny dalszy pomiar latencji i artefaktów podczas kręcenia potencjometrami na rzeczywistym interfejsie audio Windows oraz porównanie z fizycznymi kostkami. Testy nie potwierdzają wierności wszystkich wewnętrznych obwodów.

Dokumentacja i testy nie mają na celu potwierdzenia importu/elektrycznej równoważności arbitralnego obrazka: program wymaga wiarygodnej netlisty strukturalnej lub własnych metadanych.
