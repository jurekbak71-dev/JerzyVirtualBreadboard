# Aktualizacja CAD i przyrządów — 7 listopada 2026

Repozytorium: [JerzyVirtualBreadboard](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard). Aktualny kod aplikacji w commicie `541c4e3599430f58b0b02e9e014312f4d75ba45b`. Binarne archiwa Windows są publikowane dopiero po pomyślnym zakończeniu workflow [Native Windows x64](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard/actions/workflows/windows.yml); należy dobierać artefakt do SHA commitu, nigdy nie zastępować go starszym.

## Obsługa

1. `Nowa płytka` tworzy fizyczną płytkę z pustymi otworami. Przeciągaj przyciski elementów z katalogu na wybrany otwór lub kliknij, aby wstawić część do pierwszego dostępnego miejsca. Podczas przeciągania gotowej części podgląd przeskakuje między miejscami osadzenia nóżek. Zajęte/nieprawidłowe pozycje są przyciemniane, po upuszczeniu sprawdzane są kolizje i faktyczna elektryczna topologia.
2. Kliknij **prawym przyciskiem** na dziurkę: zworka od tego miejsca lub sonda napięcia. Prawym na zworkę: usuń tę zworkę. Prawym na komponent: zmiana parametrów, odwrócenie nóżek 2-pinowych (w tym A/K diody), usunięcie pojedynczej części lub pomiar prądu przez rezystor. Zmiana pozycji i usunięcie zworek zmienia netlistę solvera.
3. **Płytka / Schemat ideowy / Porównanie**: schemat jest generowany z faktycznego grafu elektrycznego. Symbole obejmują R, C, L, diody, tranzystory, napięcie i opamp; połączenia są reprezentowane wspólnymi **etykietami NET**. To nie jest jeszcze ręczny edytor tras przewodów ani pełny autorouter. `Narzędzia dodatkowe → Schemat SVG` pozwala zapisać diagram w wektorach.
4. **Oscyloskop i mierniki**: w trakcie **LIVE** wybierz węzeł z listy lub sondę prawym kliknięciem na fizyczny otwór jego listwy. Most WebView2 wysyła `VBAL_PROBE` do silnika WASAPI; solver MNA publikuje próbki napięcia ze wskazanego węzła i pomiar prądu po wskazaniu rezystora. 128 próbek odświeżane jest w panelu; obok jest miniaturowa wskazówka V / mA. Pomiary prądu innych komponentów, oscyloskop wielokanałowy, poziomowe/offsetowe pomiary dokładne i synchronizacja wyzwalania nie są jeszcze zrealizowane.
5. **Gotowe układy**: rozwijana biblioteka i opisy eksperymentów: trójpasmowy RC, Fuzz Face Ge PNP z −9 V, klipping Si/Ge/LED, NPN boost, opamp ±9 V, LPF/HPF. Zmiana C, R, diody, hFE, MOSFET Vth, JFET |Vto| i zasilania wpływa na równania symulatora. Panel objaśnia wartości do zmiany, przycisk **Schemat przykładu** przełącza na diagram.
6. **Zasilanie**: źródła DC są elementami obwodu; pasywne sieci RC i klipping diodowy mogą działać z energii sygnału wejściowego. Transystory i opampy bez żadnego źródła DC są odrzucane przez walidację frontend i backend. Obecna kontrola nie potrafi jeszcze dowieść dla każdego indywidualnego stopnia poprawnego DC bias i realnej osiągalności każdego szynowego węzła. Wartości symulacji również zależą od uproszczonych modeli urządzeń, których nie należy mylić z pełnymi modelami producenta.

## Testy

- `web/test_wiring.js` i `native/tests/realtime_circuit_tests.cpp` sprawdzają odtwarzanie i zmianę fizycznych sieci, snap, obsługę sond, generację schematu, zasilanie i moduły solvera. Weryfikować wyniki na aktualnym SHA w GitHub Actions.
- Rzeczywiste audio WAV przez ngspice: `native/tests/real_effects_integration.py` sprawdza zmianę C w EQ, Si/Ge oraz −9/−6 V w Fuzz Face.
- **Ograniczenia:** rzeczywisty odsłuch na interfejsie audio użytkownika i dokładność części rzeczywistych nie zostały potwierdzone; GUI testowano także w WebView2 smoke, nie przeprowadzono jeszcze testów użyteczności wszystkich gestów.

## Źródła i dalsze urządzenia

- [Fritzing — synchronizacja widoków](https://fritzing.org/learning/get-started/project-view/) i [obsługa konektorów](https://github.com/fritzing/fritzing-docs/blob/master/01_quickstart/03_breadboardview.md): osobny fizyczny breadboard i schemat, zaznaczanie sieci.
- [KiCad — ngspice](https://www.kicad.org/discover/spice/): sondy V/I, transient, operating point, AC/DC.
- [Falstad — scope z menu kontekstowego](https://physicslabs.ucd.ie/documentation/Falstad_circuit_sim/).
- [PT2399 — karta producenta](https://www.princeton.com.tw/en-us/Products/Multi-Media-Audio/Echo-Processor/EP-PT2399): rzeczywisty delay wymaga modelu układu CMOS z wewnętrznym ADC, RAM 44 Kbit, DAC i VCO zależnym od zewnętrznego rezystora.
- [MN3007 — dokumentacja BBD](https://www.ka-electronics.com/images/pdf/Panasonic_BBD.pdf): chorus/flanger potrzebuje **1024-stopniowej** linii opóźniającej z dwiema fazami zegara, jej biasem i filtrami. Te **układy scalone nie zostały jeszcze zaimplementowane** i nie wolno podmieniać ich atrapami DSP w przykładach.

---

# Vbreadboard — fizyczny CAD i symulacja audio (stan 7 listopada 2026)

**Pobierz** Windows x64 ZIP z [GitHub Actions](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard/actions/runs/42359861084). Potwierdzony kod binarny: commit `273de1aabedc079cef8e3e7f2c3b7c9c7c6e9905` (Windows i Linux CI **SUCCESS**). Archiwum zawiera `Vbreadboard uruchom.exe`, WebView2Loader.dll, edytor HTML, ngspice, narzędzia renderujące WAV. **Rozpakuj całe archiwum**, zachowując podfoldery; uruchom dwuklikiem `Vbreadboard uruchom.exe`. Windows WebView2 Runtime wymagany.

## Co jest fizycznie symulowane

- Każdy dyskretny element jest częścią tego samego grafu elektrycznego. Połączenia A–E i F–J realizowane są przez wewnętrzne listwy, a pomiędzy nimi użytkownik wkłada zworki. Wybór elementu pokazuje jego piny i pozwala je przełożyć; przeciągnięcie całej części przemieszcza komplet nóżek zgodnie z rastrami otworów i kontrolą kolizji.
- Biblioteka komponentów: rezystory, kondensatory, cewki, diody Si/Ge/Schottky/LED/Zener, przełączniki, 3-nóżkowe potencjometry, tranzystory NPN/PNP (odrębna uproszczona technologia germanowa dla PNP), N-MOSFET, N-JFET, opamp behawioralny i źródła DC. Indywidualne parametry BJT beta, NMOS Vth i NJFET |Vto| są przenoszone zarówno do natywnego solvera, jak i do odpowiednich modeli SPICE.
- Przykłady rzeczywistych sieci: **trójpasmowy pasywny EQ RC**, **dwa tranzystory PNP germanowe Fuzz Face z −9 V i zmianą zasilania**, **tranzystor NPN z clippingiem germanowym**, RC LPF/HPF, krzemowy/LED clipping, stopnie wzmocnienia. Wybór tych przykładów tworzy realne komponenty z przypisanymi otworami i zworkami.
- Parametry w inspektorze po zmianie aktualizują netlistę. Aktywna sesja WASAPI live jest zatrzymywana i uruchamiana ponownie z nowym modelem, aby można było porównać dźwięk po zmianie pojemności, technologii diody, napięcia lub wzmocnienia tranzystora.
- GUI ma rozwijane grupy: elementy, źródła i szyny, przykłady, pliki, magnetofon, audio live, netlista, plan montażu.

## Testy potwierdzone

- GitHub Actions Linux: 5 testów C++/CTest + statyczna składnia JS + 40 bloków regresji editor/physical board + fizyczne renderowanie plików WAV przez ngspice.
- End-to-end WAV/ngspice, przebieg sinusoidalny 1200 Hz: zmianę C1 w korektorze 100 nF → 10 nF zmierzono jako RMS **1726 → 3717** w jednostkach 16-bit PCM; wymiana diod Ge → Si RMS **5410 → 13166**; dla Fuzz Face zmiana zasilania −9 V → −6 V RMS **26605 → 19681**. Te wielkości dotyczą wyłącznie określonego wektora testowego, a nie gwarantowanych poziomów sprzętu.
- GitHub Actions Windows na commicie binarnym: sukces kompilacji x64, testów C++, ngspice WAV, startu WebView2 i mostu JavaScript↔C++; **rzeczywisty odsłuch na interfejsie muzycznym użytkownika nie został potwierdzony**.

## Ważne ograniczenia

- **Delay, chorus, flanger, reverb, compressor, tremolo i EQ macro** poprzednio były algorytmami DSP pod pozornym schematem. Są obecnie **wyłączone w bibliotece przykładów**. Nie są zrealizowanymi fizycznymi obwodami, dlatego nie udajemy, że funkcjonują w trybie elektronicznego CAD. Moduły kodu makr nadal istnieją w silniku i wymagają całkowitego zastąpienia/rozbicia na odpowiednie modele komponentów, jeżeli mają być wspierane zgodnie z docelowym wymogiem.
- Tranzystor germanowy AC128 jest na razie **uproszczonym modelem** przybliżającym podstawowe parametry Is, Bf, Vaf i Eg; nie przeszedł walidacji pomiarowej konkretnej partii elementu. Idealizowane są również op-ampy; nie są to zaimportowane pełne modele producenta.
- Wyniki generowania WAV przez pełny ngspice są zweryfikowane; silnik WASAPI realtime liczy oddzielne, uproszczone równania MNA i potrzebuje pomiarowego porównania z ngspice, zwłaszcza dla trudnych obwodów nieliniowych, stabilności i opóźnienia.

Źródła modeli i topologii: [ngspice](https://ngspice.sourceforge.io/docs/ngspice-manual.pdf), [ElectroSmash Fuzz Face](https://www.electrosmash.com/fuzz-face), [KiCad Spice Library BJT](https://github.com/kicad-spice-library/KiCad-Spice-Library), [Texas Instruments TL072](https://www.ti.com/product/TL072). Same odnośniki do producentów nie oznaczają importu ich wszystkich modeli.

---

## Fizyczny breadboard i biblioteka efektów — aktualizacja 06.11.2026

- Edytor startuje w **trybie fizycznym**. Elementy są osadzane nóżkami w konkretnych otworach A–J; połączenia powstają automatycznie przez wewnętrzne pięciootworowe listwy A–E lub F–J. Szyny T+/T-/B+/B- są osobnymi listwami i mogą mieć niezależne segmenty.
- **Kable na płytce są zworkami** między wolnymi otworami, a nie liniami pomiędzy symbolami na schemacie. Kliknij pierwszy i drugi wolny otwór, aby dodać zworkę; kliknij ją, aby usunąć. Ctrl+klik otworu podświetla listwę.
- Zmiana fizycznego otworu nóżki (wybór elementu i przycisku pinu w panelu) zmienia **elektryczną topologię**, którą otrzymuje eksport SPICE i solver live audio. Zmiana topologii zatrzymuje aktualny odsłuch, aby nie korzystał ze starego modelu.
- Nowe elementy są osadzane na istniejącej płytce bez usuwania pozostałych części i zworek. Usuwanie elementu zachowuje położenie innych części.
- Format zapisu projektu **JSON schema 5** zawiera rzeczywiste położenia nóg i zworek, konfigurację szyn oraz wybrany efekt audio macro; wczytywanie zachowuje obsługę starszych wersji JSON. Eksport planu otworów jest odświeżany po zmianach.
- Biblioteka elementów obejmuje: R, C (wyświetlanie wyłącznie w faradach jako zwykły zapis dziesiętny), L, SW, potencjometr 3-pin, diody Si/LED/Schottky/Ge/Zener, NPN/PNP/NMOS/NJFET, op-amp z DIP-8 i zasilanie DC.
- Biblioteka **16 przykładów**: clean, overdrive Si, distortion LED, fuzz Ge, op-amp boost, NPN boost, LPF/HPF oraz compressor, delay, chorus, tremolo, EQ, reverb i flanger. Uwaga: efekty dynamiki i czasu są obecnie **makrami DSP** na żywym torze audio, nie odpowiednikami fizycznych obwodów z elementów pasywnych i aktywnych.
- Sprawdzono w bieżącej sesji **34 bloki testów logiki JS** obejmujące połączenia fizyczne, zworki, zapis/odczyt, wszystkie 16 typów elementów i 16 przykładów efektów. Testy uruchomiono w izolowanym środowisku JS (mock WebView2/DOM), **nie** jako Windows GitHub Actions ani rzeczywisty odsłuch z interfejsu WASAPI.
- **Dostarczenie binarne:** workflow GitHub Actions na repozytorium zawiera kompilację Windows i pakiet programu. Po powyższych zmianach nie potwierdzono jeszcze sukcesu aktualnego przebiegu CI ani nowego artefaktu instalacyjnego. Nie należy oznaczać nowego EXE jako zweryfikowanego bez wglądu w wynik builda.

---

## Aktualizacja: zasilanie i elementy aktywne — 06.11.2026

- Dodano osobny panel **Zasilanie** z regulowanym **+V**, **−V** i **±V** oraz presetami **5 / 9 / 12 / 18 V**.
- Masa każdego źródła jest automatycznie łączona z GND; zacisk zasilania użytkownik prowadzi przewodem do układu.
- Solver realtime obsługuje stałe węzły napięciowe zasilane źródłami DC względem masy.
- Dodano **potencjometr 3-pin** z rezystancją całkowitą i pozycją suwaka; w solverze/SPICE rozkłada się na dwa rezystory.
- Dodano pierwszy aktywny element **NPN 3-pin (C/B/E)** z modelem SPICE NPNGEN oraz modelem realtime.
- Dodano preset **NPN common-emitter 9 V** z biasem, rezystorem kolektorowym i kondensatorami sprzęgającymi.
- Kondensatory pozostają prezentowane jako zwykłe wartości w **faradach**, np. `0.0000001 F`, bez notacji `1e-7`.
- Windows x64 i Linux CI dla aktualnego stanu: PASS.

## Aktualizacja live audio i elementów — 06.11.2026

- Audio live ma wybór urządzenia wejściowego i wyjściowego WASAPI.
- Ustawienia wejścia: **Stereo / Mono L→L+R / Mono R→L+R**, Input Gain, Drive przed obwodem, Output Gain i BYPASS.
- Domyślnie Drive = +12 dB i Output = -12 dB, aby mocniej pobudzić elementy nieliniowe bez podnoszenia poziomu układów liniowych o 12 dB.
- BYPASS jest odniesieniem unity względem wejścia po Input Gain i nie jest ściszany przez kompensację Output Gain.
- Dodano mocno słyszalne presety: RC 159 Hz oraz clipping silicon/Schottky/germanium/LED.
- Katalog działających elementów: rezystor, kondensator, cewka, przełącznik ON/OFF, dioda krzemowa, LED, Schottky, germanowa i Zener 5.1 V.
- Przełącznik ma rzeczywisty stan otwarty/zamknięty w solverze realtime i eksporcie SPICE.
- Schottky, germanowa i Zener mają osobne modele nieliniowe, a nie alias zwykłej diody.

## Aktualizacja: wejście audio, NEW i większy breadboard

- Główny plik uruchamiający Windows ma nazwę **`Vbreadboard uruchom.exe`**.
- Magnetofon ma osobne kroki **Wczytaj MP3/WAV** i **PLAY**. Sam wybór pliku nie wymaga już wcześniej poprawnego obwodu.
- Dekoder używa natywnej szerokiej ścieżki Windows, więc obsługuje nazwy/katalogi z polskimi znakami.
- Dodano **NEW** i **Usuń wszystkie elementy**.
- Breadboard można przełączyć na **30, 60, 90 albo 120 kolumn**; większe płytki są przewijane poziomo, a rozmiar zapisuje się w projekcie.
- Katalog elementów rozszerzono o **cewkę L** i **LED**. Oba elementy mają rzeczywistą obsługę w solverze realtime i w eksporcie SPICE; LED używa modelu LEDGEN.
- Tranzystory, potencjometry 3-pin i wzmacniacze operacyjne nie są jeszcze dostępne — wymagają wielopinowego modelu komponentów i osobnego rozszerzenia solvera.

# Virtual Breadboard Audio Lab

Publiczny projekt badawczo-rozwojowy Windows x64: symulacja rzeczywistych obwodów elektronicznych i przetwarzanie sygnałów audio.

## Najnowszy etap: WebView2 → C++ → ngspice → WAV

W `vbal_breadboard.exe` przycisk **„Symuluj WAV w ngspice”** uruchamia symulację aktualnie narysowanego obwodu, bez ręcznego eksportu netlisty.

1. Wybierz preset lub narysuj własne połączenia elementów R/C/D.
2. Kliknij **Symuluj WAV w ngspice**.
3. W systemowym oknie wybierz WAV PCM.
4. Po zakończeniu obok wejścia pojawi się `NAZWA_BREADBOARD.wav`.

Program przesyła z UI do C++ wyłącznie definicje R/C/D, waliduje nazwy, wartości i spójność obwodu, generuje netlistę oraz wykonuje solver ngspice. Niedozwolone polecenia SPICE są odrzucane. Limit: maks. 500000 ramek na kanał. Interfejs podczas symulacji działa synchronicznie, zatem może chwilowo nie odpowiadać przy dużych plikach. Symulacja odbywa się **offline**, nie w czasie rzeczywistym.

CI Windows weryfikuje rzeczywisty solver dla różnych topologii oraz komunikację JS → WebView2 → C++. Aplikacja musi być rozpakowana wraz z katalogiem `ngspice/` i `web/`.

## Audio live — aktualny etap

W `vbal_breadboard.exe` jest pierwszy natywny tor realtime: **domyślne wejście WASAPI → solver obwodu R/C/D → domyślne wyjście WASAPI**. Warstwę urządzeń zapewnia przypięty miniaudio 0.11.26; sam obwód liczy własny solver C++ bez uruchamiania ngspice w callbacku audio.

- `START LIVE AUDIO` ładuje bieżącą topologię z breadboarda do solvera realtime.
- `STOP LIVE` zatrzymuje duplex.
- Format roboczy: 48 kHz, stereo, float32, okres 128 ramek / 3 bufory.
- Kondensatory zachowują stan między próbkami; diody są rozwiązywane iteracyjnie jako elementy nieliniowe.
- Panel Audio live pokazuje peak wejścia/wyjścia i stan solvera.
- Zmiana obwodu zatrzymuje live, aby nie odtwarzać starej topologii po edycji.
- Solver ma test odpowiedzi RC, clippingu diodowego, układu wielowęzłowego i test szybkości > realtime.

**Ważne:** GitHub CI potwierdza kompilację Windows i testy solvera, ale runner nie ma fizycznej karty wejściowej. Faktyczny odsłuch WASAPI wymaga testu na komputerze użytkownika.

## Co jest zaimplementowane (06.11.2026)

- **Natywne GUI Windows (`vbal_gui.exe`)** uruchamiane dwuklikiem; wybór WAV, parametry RC, przycisk przetwarzania.
- **Trzy tryby**: filtr referencyjny C++, symulacja RC przez ngspice, symulacja RC z dwiema przeciwsobnymi diodami (nieliniowy clipping).
- **Nowość P2:** niezależny model elektryczny C++ dla rezystorów, kondensatorów i diod, z walidacją węzłów i generowaniem schematu SPICE, BOM CSV oraz opisu połączeń.
- **Nowość P3:** interaktywny edytor HTML `web/editor.html` osadzony w samodzielnym `vbal_breadboard.exe` przez Microsoft WebView2 i uruchamiany z `vbal_gui.exe` przyciskiem „Otworz breadboard”. Obsługa przesuwania komponentów, edycji wartości i **niezależnych przewodów pomiędzy konkretnymi zaciskami**, usuwania przewodów przez kliknięcie oraz przyciskami cofania i czyszczenia, zapisu/odczytu projektów JSON i eksportu SPICE/BOM.
- **P4:** edytor jest połączony z natywnym C++/ngspice — przycisk **„Symuluj WAV w ngspice”** symuluje aktualnie zbudowany graf R/C/D bez ręcznego eksportowania netlisty.
- **P5 — import:** wczytywanie netlist **SPICE/KiCad/LTspice** (`.cir/.sp/.spi/.net/.txt`) oraz podstawowego pośredniego netlistu **KiCad XML**. Importowane R/C/D, wartości z sufiksami inżynierskimi i węzły są odtwarzane jako edytowalne elementy i przewody. Elementy nieobsługiwane są odrzucane zamiast pomijane.
- **P5 — breadboard fizyczny:** po imporcie program automatycznie przypisuje zaciski do konkretnych grup pięciootworowych, używa dolnej szyny „−” jako masy, rezerwuje osobne otwory dla IN/OUT/GND, sprawdza kolizje i pozwala wyeksportować plan montażu z oznaczeniami otworów.
- Audio PCM WAV 16/24/32 bit na wejściu, PCM16 na wyjściu, obsługa mono/stereo.
- **Prawdziwy ngspice 47** w paczce Windows GitHub Actions, dostarczany jako zewnętrzny solver z kompletem plików.
- Źródło PWL dla maks. 4096 ramek; **XSPICE filesource** dla dłuższych nagrań do 500000 ramek na kanał (np. ok. 10,4 s przy 48 kHz).
- Testy C++ i testy integracyjne WAV → SPICE → WAV w GitHub Actions Linux i Windows.

To nadal **prototyp offline**, nie gotowa płytka stykowa ani symulator czasu rzeczywistego. Przetwarzanie SPICE może przez chwilę blokować okno GUI.

## Test Windows — bez instalacji kompilatora

1. Otwórz [GitHub Actions](https://github.com/jurekbak71-dev/JerzyVirtualBreadboard/actions) i ostatni zakończony sukcesem przebieg **Native Windows x64**.
2. Pobierz artefakt **VBAL-Windows-SPICE-GUI-x64**, rozpakuj cały ZIP, **zachowując katalog `ngspice/`**.
3. Uruchom `vbal_gui.exe`. Wybierz PCM WAV, parametry RC i kliknij **Przetworz WAV**.
4. Tryb domyślny zapisuje `nazwa_RC.wav`.
5. Zaznacz **Symuluj fizyczny obwod w ngspice**, aby zapisać `nazwa_SPICE_RC.wav`.
6. Zaznacz **Dwie przeciwsobne diody**, aby zapisać `nazwa_SPICE_DIODE.wav`. Porównaj nagrania.
7. Kliknij **Eksportuj obwod + BOM** dla plików `.cir`, `_BOM.csv` i `_connections.txt`.
8. Kliknij **Otworz breadboard**: uruchomi się samodzielne okno `vbal_breadboard.exe` z osadzonym WebView2, bez zewnętrznej przeglądarki. **Dodawanie przewodów:** kliknij pierwszy zacisk (np. IN), potem drugi (np. R1.a). Kliknij przewód, aby go usunąć.
9. **Import z programu EDA:** użyj **Import SPICE / KiCad / LTspice**. W KiCad 9 można użyć File → Export → Netlist… → SPICE. Import automatycznie przechodzi do pierwszego fizycznego rozmieszczenia na breadboardzie.
10. Alternatywnie użyj **Import KiCad XML netlist** dla podstawowego pośredniego XML KiCad.
11. **Rozmieść na płytce** tworzy fizyczny plan otworów; **Eksport planu otworów** zapisuje go jako tekst.
12. **Symuluj WAV w ngspice** uruchamia aktualnie zaimportowany/narysowany obwód przez C++ i ngspice.

Nie uruchamiaj `vbal_rc.exe` dwuklikiem: to narzędzie konsolowe. Do testów okienkowych służy `vbal_gui.exe`.


### Import z KiCad

KiCad 9 potrafi eksportować schemat do netlisty SPICE przez **File → Export → Netlist… → SPICE**. Ten format jest obecnie preferowaną drogą importu do VBAL. KiCad normalizuje wartości elementów pasywnych do składni zgodnej ze SPICE (np. `4.7kΩ → 4.7k`, `4u7 → 4.7u`).

### Znane ograniczenia

- ngspice offline, zewnętrzny proces; nie ma monitoringu audio w czasie rzeczywistym.
- Maksymalnie 500000 ramek na kanał dla trybu SPICE; pełne utwory wymagają przyszłego streamingu/chunkingu.
- Dostępny jest pierwszy **automatyczny planer fizycznych otworów breadboarda** z walidacją kolizji, ale nie jest to jeszcze pełny autorouter dla dowolnej płytki. Bieżący planer jest przeznaczony dla małych układów R/C/D i jednej konkretnej logicznej topologii grup 5-otworowych + wspólnej szyny masy.
- Import obrazu/skanu schematu (PNG/JPG/PDF) **nie jest jeszcze zaimplementowany**. Aktualny import jest maszynowo czytelny: SPICE oraz podstawowy KiCad XML.
- Modele diod importowanych z zewnętrznych netlist są obecnie mapowane do generycznego modelu DGEN; konkretny model producenta nie jest jeszcze zachowywany.
- Symulowany układ diodowy jest przykładowym obwodem z generycznym modelem diody, nie zweryfikowanym modelem konkretnego producenta.
- Poprawność elektryczna i bezpieczeństwo fizycznego montażu wymagają niezależnej walidacji.

### WebView2 Runtime

Aplikacja korzysta z Microsoft Edge WebView2 Runtime (Evergreen), standardowo obecnego na Windows 11 i większości aktualnych Windows 10. To komponent systemowy; użytkownik **nie musi otwierać Edge ani innej przeglądarki**. Jeśli Runtime nie istnieje, potrzebna jest jego oficjalna instalacja: https://developer.microsoft.com/en-us/microsoft-edge/webview2/.

**Weryfikacja:** Windows CI kompiluje i uruchamia `vbal_breadboard.exe`; testy funkcjonalne przewodów działają w Node.js na kodzie edytora.

## Budowanie ze źródeł

Windows — Developer PowerShell (Visual Studio 2022 + C++):

```powershell
cmake -S native -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

CLI, model matematyczny RC:

```powershell
.\build\Release\vbal_rc.exe input.wav output.wav 10000,1e-8
```

CLI, rzeczywista symulacja ngspice:

```powershell
.\build\Release\vbal_render_spice.exe input.wav output.wav 10000,1e-8 rc
.\build\Release\vbal_render_spice.exe input.wav diode.wav 10000,1e-8 diode
```

Przy samodzielnym uruchamianiu CLI ngspice na Windows skonfiguruj `PATH` dla `ngspice_con.exe` oraz `SPICE_LIB_DIR` wskazujące `Spice64/share/ngspice`. W paczce GUI wykrywanie katalogów wykonywane jest automatycznie.

## Etapy docelowe

1. Symulacja i przepływ audio (offline → realtime).
2. Silnik modeli rzeczywistych części i spójny schemat elektryczny.
3. Breadboard z przewodami, weryfikacją połączeń i instrumentami pomiarowymi.
4. Eksport schematów montażowych, pinoutów i BOM.
5. Na końcu: generowanie kodu DSP/VST3/Raspberry Pi.

## Źródła i komponenty zewnętrzne

- [ngspice 47 — oficjalne wydanie Windows](https://sourceforge.net/projects/ngspice/files/ng-spice-rework/47/) (ngspice 47, SHA-256 archiwum `59225971bd68cdd1199443649aa4615a9e6d684933f205ab49006a3942518f5a`).
- [ngspice — źródła](https://github.com/ngspice/ngspice), [licencja](https://github.com/ngspice/ngspice/blob/master/COPYING).
- [Plan i decyzje projektu w Notion](https://app.notion.com/p/4105fed1901b810198f7f7c008eedbf9).

## Bezpieczeństwo

Wynik symulacji nie gwarantuje bezpieczeństwa rzeczywistego układu. Przed budową należy sprawdzić zasilanie, polaryzację, limity komponentów, izolację i zgodność pinów z dokumentacją.
