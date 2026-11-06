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
