# Źródła schematów efektów do weryfikacji w Vbreadboard
Stan przeglądu: 2026-11-07. Statusy nie oznaczają importu, uruchomienia ani zgodności modeli SPICE.

| Źródło | Zakres | Sprawdzone | Dalsze działania | Uwagi prawne |
|---|---|---|---|---|
| https://tagboardeffects.blogspot.com/ | Płytki stripboard/vero | Bezpośredni dostęp: HTTP 429 | Każdą topologię sprawdzić w schemacie i BOM | Nie kopiować grafiki |
| https://parasitstudio.com/resources/layouts/ | Fuzz, distortion, filtry, chorus PT2399, układy użytkowe | Przeczytano indeks layoutów i deklarację weryfikacji z wyjątkami | Sprawdzać schemat ideowy, pinout i wartości przed importem | Linkować oryginały, szanować prawa autorów |
| https://generalguitargadgets.com/how-to-build-it/technical-help/schematics/ | Fuzz Face, Big Muff, TS-808, PT80 Echo, AD3208, chorus, kompresory | Potwierdzono indeks i obejrzano schemat AD-3208 | Przekształcać zweryfikowaną topologię we własny graf elektryczny | Autor dopuszcza linkowanie, zabrania kopii swoich plików na innych witrynach |
| https://www.freestompboxes.org/viewforum.php?f=13 | Projekty DIY, analizy, poprawki i schematy | Bezpośredni dostęp HTTP 403; odczytano indeks wyszukiwarki | Sprawdzić konkretne wątki, autorstwo, wersje i poprawki | Forum zastrzega zgodę autorów przy zastosowaniu komercyjnym |
| https://www.princeton.com.tw/en-us/Products/Multi-Media-Audio/Echo-Processor/EP-PT2399 | Specyfikacja PT2399, model mixed signal, DIP16, zegar pin 6 | Potwierdzono architekturę i tabelę czasów opóźnienia | Osobny testowany model funkcjonalny układu w solverze, prawdziwy pinout i zasilanie | Dane techniczne do implementacji, schematu producenta nie redystrybuować bez uprawnień |

## Priorytety importu do CAD

1. Układy analogowe bez specjalizowanych IC: proste boostery, tranzystorowe fuzzy, passive EQ. Zbudować dokładny graf węzłów, symbole, rzeczywistą płytkę i test ngspice.
2. PT2399: model wewnętrzny opóźnienia sterowany rzeczywistym rezystorem przy pinie 6; 16 wyprowadzeń, 5 V, zewnętrzne analogowe filtry i obwody odniesienia; ograniczenia napięć i czasu. Nie zastępować gotowym presetem delay.
3. BBD/MN3102 i BL3208: odwzorować piny, dwufazowy zegar, ładunki, zasilanie/bias, filtry i sprzężenie; test przebiegów i zmian wartości. Model BBD może być behawioralny wewnątrz rzeczywistego komponentu, musi zachowywać interakcje na pinach.
4. Układy z większą liczbą tranzystorów i opampów: nie importować gotowej grafiki jako dowodu poprawności, lecz przeliczać każdy element.

Kryterium akceptacji dla importu: zgodność schematu ideowego i grafu solvera; pełny BOM z wartościami i odniesieniem; walidacja polaryzacji oraz zasilania; testy AC/transient, porównanie punktów pracy i audio przy zmianach R/C/diod/VCC; oznaczenie poziomu dokładności i źródła.
