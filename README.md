# Virtual Breadboard Audio Lab

Publiczny projekt badawczo-rozwojowy Windows x64: symulacja rzeczywistych obwodów elektronicznych i przetwarzanie sygnałów audio.

## Co jest zaimplementowane (06.11.2026)

- **Natywne GUI Windows (`vbal_gui.exe`)** uruchamiane dwuklikiem; wybór WAV, parametry RC, przycisk przetwarzania.
- **Trzy tryby**: filtr referencyjny C++, symulacja RC przez ngspice, symulacja RC z dwiema przeciwsobnymi diodami (nieliniowy clipping).
- **Nowość P2:** niezależny model elektryczny C++ dla rezystorów, kondensatorów i diod, z walidacją węzłów i generowaniem schematu SPICE, BOM CSV oraz opisu połączeń.
- **Nowość P2:** interaktywny edytor HTML `web/editor.html` uruchamiany przyciskiem „Otworz breadboard” z EXE. Obsługa przesuwania komponentów, zmiany węzłów i wartości, łączenia zacisków, wczytania i zapisu JSON oraz eksportu SPICE/BOM.
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
8. Kliknij **Otworz breadboard**: otworzy się lokalny plik HTML w przeglądarce. Można w nim edytować graf węzłów i eksportować obwód. Edytor HTML nie steruje jeszcze bieżącą symulacją w EXE.

Nie uruchamiaj `vbal_rc.exe` dwuklikiem: to narzędzie konsolowe. Do testów okienkowych służy `vbal_gui.exe`.

### Znane ograniczenia

- ngspice offline, zewnętrzny proces; nie ma monitoringu audio w czasie rzeczywistym.
- Maksymalnie 500000 ramek na kanał dla trybu SPICE; pełne utwory wymagają przyszłego streamingu/chunkingu.
- Dostępny jest wczesny edytor HTML i model komponentów, **ale nie ma jeszcze integracji WebView2 ani fizycznej walidacji rozmieszczenia otworów breadboarda**. Edytor HTML działa jako osobny plik w przeglądarce. Eksportuje węzły logiczne, nie zatwierdzony montaż fizyczny.
- Symulowany układ diodowy jest przykładowym obwodem z generycznym modelem diody, nie zweryfikowanym modelem konkretnego producenta.
- Poprawność elektryczna i bezpieczeństwo fizycznego montażu wymagają niezależnej walidacji.

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
