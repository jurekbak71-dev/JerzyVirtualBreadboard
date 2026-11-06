# Virtual Breadboard Audio Lab

Publiczny projekt badawczo-rozwojowy Windows x64: symulacja rzeczywistych obwodów elektronicznych i przetwarzanie sygnałów audio.

## Co jest zaimplementowane (06.11.2026)

- **Natywne GUI Windows (`vbal_gui.exe`)** uruchamiane dwuklikiem; wybór WAV, parametry RC, przycisk przetwarzania.
- **Trzy tryby**: filtr referencyjny C++, symulacja RC przez ngspice, symulacja RC z dwiema przeciwsobnymi diodami (nieliniowy clipping).
- **Nowość P2:** niezależny model elektryczny C++ dla rezystorów, kondensatorów i diod, z walidacją węzłów i generowaniem schematu SPICE, BOM CSV oraz opisu połączeń.
- **Nowość P3:** interaktywny edytor HTML `web/editor.html` osadzony w samodzielnym `vbal_breadboard.exe` przez Microsoft WebView2 i uruchamiany z `vbal_gui.exe` przyciskiem „Otworz breadboard”. Obsługa przesuwania komponentów, edycji wartości i **niezależnych przewodów pomiędzy konkretnymi zaciskami**, usuwania przewodów przez kliknięcie oraz przyciskami cofania i czyszczenia, zapisu/odczytu projektów JSON (schemat 2, odczyt starszego schematu 1) i eksportu SPICE/BOM. Nowe elementy są odłączone, nie podłączają się automatycznie do masy ani wyjścia.
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
8. Kliknij **Otworz breadboard**: uruchomi się samodzielne okno `vbal_breadboard.exe` z osadzonym WebView2, bez zewnętrznej przeglądarki. Można w nim edytować graf węzłów i eksportować obwód. Edytor HTML nie steruje jeszcze bieżącą symulacją w EXE. **Dodawanie przewodów:** kliknij pierwszy zacisk (np. IN), potem drugi (np. R1.a); przewód pojawi się między nimi. Kliknij przewód, aby go usunąć. Nowo dodane elementy nie mają żadnych przewodów, aż je utworzysz. Presety zawierają celowo wstępnie podłączony obwód — użyj „Wyczyść przewody” do samodzielnego okablowania.

Nie uruchamiaj `vbal_rc.exe` dwuklikiem: to narzędzie konsolowe. Do testów okienkowych służy `vbal_gui.exe`.

### Znane ograniczenia

- ngspice offline, zewnętrzny proces; nie ma monitoringu audio w czasie rzeczywistym.
- Maksymalnie 500000 ramek na kanał dla trybu SPICE; pełne utwory wymagają przyszłego streamingu/chunkingu.
- Dostępny jest wczesny edytor HTML i model komponentów, a **interfejs WebView2 jest osadzony w osobnym EXE**. Nie ma jeszcze fizycznej walidacji rozmieszczenia otworów breadboarda ani połączenia projektu HTML z silnikiem ngspice w czasie działania. Eksportuje węzły logiczne, nie zatwierdzony montaż fizyczny.
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
