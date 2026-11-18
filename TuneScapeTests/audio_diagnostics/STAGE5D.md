# TuneScape — weryfikacja i raport końcowy etapu 5D

Raport z 18 listopada 2026. Gałąź `refactor/codebase-audit`, baza `e56adae`.

**Przerwana praca nie była zakończona.** Pozostały działający proces macierzy, nieopisane wyniki oraz brak CTest i raportu 5D. Po wznowieniu sprawdzono kod i istniejące artefakty, pozwolono dokończyć rozpoczętą macierz, wykonano końcowe buildy i CTest oraz uzupełniono dokumentację. Nie powtarzano macierzy ani poprawnego pełnego QtTest qmake.

**Spełniono alternatywne kryterium 5D: jednoznaczne udokumentowanie dalszych ograniczeń. Nie uzyskano potwierdzenia bezpiecznej konfiguracji wydaniowej.** Zależność od wersji Qt i urządzenia pozostaje nierozstrzygnięta, ponieważ dostępne były jeden kompletny zestaw Qt i jedno wyjście audio. Awarii natywnej nie uznano za naprawioną.

## Stan wymagań

| Wymaganie | Stan końcowy i dowód |
| --- | --- |
| Dokładne wersje, plugin i pochodzenie DLL | Ukończone. `kit.json`, inwentarz modułów w każdym z 192 logów, sumy SHA-256. Kontrola wszystkich ścieżek bez wykrytego mieszania instalacji |
| Powtarzalna seria na Qt 6.7.0 | Ukończone. 8 próbek × 2 tryby × 12 osobnych procesów = 192 próby |
| Nowszy kompletny zestaw Qt | Niewykonane warunkowe porównanie. W sprawdzonej instalacji dostępny tylko Qt 6.7.0; nie podmieniano pojedynczych DLL |
| Inne urządzenie audio | Niewykonane warunkowe porównanie. Inwentarz wykazuje jedno wyjście Realtek |
| Poprawność materiału i niezależny enkoder | Ukończone w opisanym zakresie. 8 próbek zweryfikowanych przez ffprobe i niezależny FFmpeg CLI; wszystkie sumy danych wejściowych zgodne z manifestem |
| Wyniki procesów, liczba cykli i moment zakończenia | Ukończone. Kody zakończenia, timeout, ostatni rozpoczęty cykl i ostatnia zalogowana faza w `matrix.json`; szczegółowe zdarzenia w logach |
| Crash dump / stos | Częściowo. Zachowano 22 minidumpy; wcześniejsza analiza metadanych obejmuje 10, a LLDB otworzył jeden. Brak pełnych symboli i kompletnego stosu biblioteki w wątku awarii |
| Awaria bez kodu TuneScape | Ukończone. Program diagnostyczny nie linkuje klas TuneScape; awarie także w trybie samego QAudioDecoder |
| Kolejność niszczenia player/output | Ukończone w zakresie kontroli kodu i regresji. Zachowany test 5C przeszedł; worker usuwa odtwarzacze przy nadal żywym współdzielonym wyjściu |
| Decyzja techniczna | Ukończone z ograniczeniami opisanymi poniżej. Brak dowodu pozwalającego zatwierdzić aktualny lub nowszy zestaw do stabilnego wydania |
| Windows qmake, QtTest, CMake/CTest, diff check | Ukończone. Wyniki poniżej |
| Linux | Niewykonana kompilacja i testy. WSL Ubuntu 24.04 ma CMake, lecz brak qmake6 i wymaganych pakietów developerskich Qt 6 |

## Co rzeczywiście zostało po przerwaniu

W repozytorium były trzy zmienione pliki śledzone: `CMakeLists.txt`, `audio_diagnostics.pro` i `main.cpp`. Dwa nowe skrypty PowerShell nie były jeszcze śledzone. Kod produkcyjny TuneScape nie był zmieniony względem bazy etapu 5D.

Reproduktor otrzymał odczyt pliku przez `--input`, ograniczenie jego rozmiaru, SHA-256, inwentarz faktycznie załadowanych modułów oraz parametrów urządzenia, wybór urządzenia przez `--device`, zapis fixture'a przez `--write-fixture`, opcjonalny minidump Windows i jawny kod zakończenia procesu potomnego. Dodano też opcję formatu QAudioDecoder; osobnej macierzy `--decode-format` nie wykonano.

`prepare_fixtures.ps1` przygotował dane i ich niezależną walidację. `run_matrix.ps1` uruchomił serię z osobnym procesem, logiem, timeoutem i opcjonalnym dumpem dla każdej próby. PATH i ścieżki pluginów wskazują wybrany zestaw Qt. Bezpośrednie wywołania `--resample` zachowują ograniczenie ABI FFmpeg 6 z 5C i nie służą do porównania dowolnych nowszych bibliotek.

Istniały świeże buildy Windows qmake i CMake oraz pełny wynik QtTest qmake. Macierz kontynuowała pracę pomimo przerwania rozmowy. Nie uruchomiono drugiej kopii tej serii.

Przy dokończeniu dodano ochronę obu skryptów przed ponownym użyciem niepustego katalogu wynikowego. Nazwy wyników dla jawnego urządzenia używają skrótu jego całego identyfikatora, ponieważ identyfikatory Windows mogą mieć wspólny długi prefiks. Zmiany te nie zmieniły danych wcześniejszej macierzy. Sprawdzono składnię skryptów i odmowę nadpisania istniejących wyników; sumy manifestów przed i po sprawdzeniu pozostały identyczne.

## Porównywane środowisko

| Składnik | Rzeczywista konfiguracja |
| --- | --- |
| System | Windows 11 x64 |
| Qt / Qt Multimedia / plugin FFmpeg | 6.7.0 / 6.7.0 / 6.7.0.0 |
| Kompilator aplikacji i reproduktora | MinGW GCC 11.2.0 |
| FFmpeg załadowany przez Qt | 6.1.1 |
| avcodec / avformat / avutil | 60.31.102 / 60.16.100 / 58.29.100 |
| swresample | 4.12.100, `swresample-4.dll` |
| Lokalizacja zestawu | `C:/Qt/6.7.0/mingw_64` |
| Dostępne wyjście audio | Realtek, domyślne, preferowane stereo / 48000 Hz / Float |
| Osobny enkoder i walidator CLI | FFmpeg 9.0.2 essentials build, www.gyan.dev |

**FFmpeg CLI 9.0.2 służył wyłącznie do tworzenia i niezależnego sprawdzania plików. Nie był backendem Qt.** Nie wykonano porównania Qt z FFmpeg 6.1.1 względem Qt z FFmpeg 9.0.2. Biblioteki Qt/FFmpeg zapisane we wszystkich logach pochodzą z jednego zestawu 6.7.0.

Inwentarz obejmuje ścieżki modułów rzeczywiście załadowanych do procesu, a nie tylko deklarację PATH. Sprawdzono zgodność wszystkich 192 logów. `kit.json` zawiera sumy bibliotek; plik `stage5d-results.json` zachowuje inwentarz, parametry próbek, agregaty oraz sumy kluczowych artefaktów.

## Materiał i pełna macierz

Niezależnie kodowany materiał to generowana sinusoida 997 Hz, czas źródła 6 s. MP2/MP3 mają zadany bitrate 128 kbps. Próbki syntetyczne zachowano z 5C, bez zastępowania ich nowymi danymi. FFprobe potwierdził kodek, kanały i częstotliwość. Każda próbka przeszła niezależne dekodowanie CLI z `-xerror` i kodem 0. Kontrola SHA-256 wszystkich ośmiu plików nie wykazała zmian. Jest to walidacja praktyczna, a nie formalny dowód zgodności każdego bitu ze specyfikacją MPEG.

| Próbka | Kanały / Hz | Format próbek raportowany przez ffprobe | Worker: awarie / próby | QAudioDecoder: awarie / próby |
| --- | --- | --- | --- | --- |
| Syntetyczne MP2 mono z 5C | 1 / 44100 | fltp | 4/12 | 6/12 |
| Syntetyczne MP3 stereo z 5C | 2 / 44100 | fltp | 0/12 | 0/12 |
| Niezależnie kodowane MP2 mono | 1 / 44100 | fltp | 0/12 | 3/12 |
| Niezależnie kodowane MP2 mono | 1 / 48000 | fltp | 3/12 | 6/12 |
| Niezależnie kodowane MP2 stereo | 2 / 44100 | fltp | 0/12 | 0/12 |
| Niezależnie kodowane MP3 mono | 1 / 44100 | fltp | 0/12 | 0/12 |
| Niezależnie kodowane MP3 stereo | 2 / 44100 | fltp | 0/12 | 0/12 |
| Niezależnie zapisane PCM WAV mono | 1 / 44100 | s16 | 0/12 | 0/12 |

Łącznie **192 procesy, 170 zakończeń z kodem 0, 22 zakończenia z 0xC0000005, zero timeoutów, 22 niepuste minidumpy**. Sprawdzono istnienie wszystkich logów i dumpów odpowiadających awariom. Nie uruchamiano dodatkowych serii w poszukiwaniu następnej sekwencji powodującej błąd.

Worker miał zadane 12 cykli, szybkie zmiany stacji oraz dodatkowe zamknięcie aktywnego dekodera. Liczba cykli zadanych nie oznacza liczby ukończonych w procesie, który uległ awarii. `LastCycle` jest indeksem ostatniego rozpoczętego cyklu, a `LastPhase` ostatnim komunikatem przed zakończeniem; asynchroniczność uniemożliwia traktowanie tego komunikatu jako dowodu operacji sprawczej. Tryb QAudioDecoder dekoduje plik i nie wykonuje 12 cykli odtwarzania.

Te proporcje opisują wyłącznie skończoną serię na tej maszynie. Nie są oszacowaniem częstości awarii użytkowników ani dowodem bezpieczeństwa wariantów z zerem awarii.

## Stos, możliwe przyczyny i ograniczenia dowodowe

Zachowany wynik analizy metadanych 10 wcześniejszych dumpów wskazuje wyjątek `0xc0000005` w załadowanym `swresample-4.dll`. LLDB otworzył jeden dump workera i wskazał tę samą bibliotekę w wątku awarii. Nazwa `swresample_license + ...` jest nazwą najbliższego dostępnego eksportu z przesunięciem. Nie dowodzi wykonywania funkcji odpowiedzialnej za licencję. Nie uzyskano pełnego wiarygodnego rozwinięcia stosu tego wątku do kodu źródłowego Qt i FFmpeg.

Nie rozszerzano analizy zrzutów o wyszukiwanie warunków następnej awarii. Zachowano istniejące pliki LLDB i metadanych. Odczytane metadane obejmują 10 dumpów, a nie wszystkie 22. Zgodny kod wyjątku wszystkich procesów nie wystarcza do stwierdzenia, że wszystkie awarie mają jedną przyczynę.

| Czynnik | Wniosek uzasadniony danymi | Czego nie wykazano |
| --- | --- | --- |
| Kod aplikacji | GUI, RadioList i PlaybackController nie są potrzebne do wystąpienia awarii samodzielnego reproduktora | Że każda awaria TuneScape pochodzi z tej samej przyczyny lub że kod aplikacji jest całkowicie wolny od błędów |
| Qt Multimedia | Integracja dekodera, konwersji PCM, wątków i czasu życia obiektów pozostaje możliwym źródłem problemu | Konkretnego błędu Qt ani usunięcia go w nowszej wersji |
| FFmpeg | Biblioteka konwersji audio jest miejscem zaobserwowanego wyjątku | Czy winne są jej własny kod, parametry przekazane przez Qt, czas życia buforów czy wcześniejsze uszkodzenie pamięci |
| MP2 mono | Korelacja nie ogranicza się do syntetycznego fixture'a; dotyczy również niezależnie zakodowanych danych | Że wszystkie MP2 mono są wadliwe, że samo mono jest przyczyną lub że każdy MP3 jest bezpieczny |
| Urządzenie i format wyjściowy | Wyjście stereo 48 kHz może wymagać konwersji kanałów, częstotliwości i reprezentacji PCM | Wpływu innego urządzenia lub innego sterownika; nie wykonano takiego porównania |
| QAudioDecoder bez wyjścia | Awaria występuje bez aktywnego QMediaPlayer, QAudioOutput i transmisji HTTP | Całkowitego wykluczenia wpływu środowiska audio — program nadal enumeruje urządzenia w inwentarzu |
| Wersje bibliotek | Potwierdzono jedną spójną konfigurację Qt 6.7.0/FFmpeg 6.1.1 | Zależności przyczynowej od wersji bez porównania z innym kompletnym zestawem |

MP2 to kodowanie skompresowanego strumienia, mono to liczba kanałów, a format PCM i parametry urządzenia to kolejne właściwości toru. Zmiana MP2 mono na MP3 stereo zmienia kilka czynników naraz. Dodane kontrole rozdzielają część tych różnic, ale nie czynią porównań idealnie jednoczynnikowymi. `fltp` opisuje reprezentację próbek dekodera, nie kodek ani format wymagany przez urządzenie.

`libswresample` obsługuje również konwersję reprezentacji próbek i układu kanałów. Jego udział nie oznacza wyłącznie zmiany częstotliwości. Awaria w kodzie natywnym pozostaje zagrożeniem dla całego procesu, niezależnie od poprawności stanu GUI. Izolacja procesów dotyczy testów i nie zabezpiecza produkcyjnej aplikacji.

W workerze współdzielone QAudioOutput powstaje przed właścicielem odtwarzaczy i jest niszczone po nim. Po udanym Stop sprawdzane są usunięcie odtwarzaczy i zamknięcie połączeń. Zachowany test integracyjny 5C sprawdza kolejność player → output również w RadioAudioManager. Nie wprowadzono nowej zmiany produkcyjnej, opóźnień synchronizacyjnych ani osobnego wyjścia na każdą sesję. Nie przeprowadzono nowego pełnego profilowania pamięci natywnej.

## Końcowa weryfikacja

| Kontrola | Rzeczywisty wynik |
| --- | --- |
| Świeży Windows qmake, aplikacja + testy + diagnostyka | Istniejący poprawny build 5D potwierdzony logiem i artefaktami. Końcowy `mingw32-make -j4` zwrócił 0, bez zaległych zmian do kompilacji |
| Pełny QtTest qmake z backendem | Zachowany `full-qmake.txt`: 84 zaliczone, 0 niepowodzeń, 1 jawnie pominięty test publicznego internetu, 37.820 s. Wynik wykorzystano ponownie |
| Świeży CMake RelWithDebInfo | Istniejący poprawny build 5D. Końcowy `cmake --build ... --parallel 4`: kod 0, `ninja: no work to do` |
| Końcowy CTest z `TUNESCAPE_TEST_MEDIA=1` | 2/2 zaliczone, 53.91 s; główny zestaw 34.18 s, program diagnostyczny 19.59 s; diagnostyka ma 8 zaliczeń i 0 niepowodzeń |
| Składnia skryptów PowerShell | Zero błędów parsera |
| Ochrona istniejących wyników | Oba skrypty odrzuciły niepuste katalogi; manifesty pozostały niezmienione |
| Spójność dowodów | 192 logi, 22 dumpy dla awarii, zgodne sumy 8 próbek, brak niezgodnych ścieżek modułów |
| `git diff --check` | Kod 0 |
| Linux | Nie kompilowano i nie uruchamiano testów. Odczyt WSL potwierdził brak wymaganych narzędzi i pakietów Qt 6 |

Końcowy CTest pokrył się czasowo z ostatnią częścią wcześniej rozpoczętej macierzy. Jego wynik jest kontrolą wykonania zestawu, a nie osobnym pomiarem częstości awarii. Nie kasuje 22 natywnych awarii z pełnej serii. Główny lokalny zestaw testów nie jest dowodem stabilności wszystkich kodeków.

## Decyzja techniczna i zakres dalszych prac

1. Zachować obecny zestaw jako bazę diagnostyczną, zidentyfikowaną wersjami i sumami. **Nie zatwierdzać na tej podstawie stabilnego wydania z deklarowaną niezawodną obsługą mono MP2.**
2. Rozpatrywać aktualizację Qt Multimedia i FFmpeg jako kompletnego, zgodnego zestawu. Brak danych uzasadniających wskazanie konkretnej nowszej wersji jako naprawionej. FFmpeg CLI 9.0.2 nie stanowi takiego dowodu.
3. Nie wprowadzać kolejnej poprawki produkcyjnej ani przełączenia backendu bez potwierdzonego uzasadnienia. Dotychczasowe poprawne zmiany 5C pozostają zachowane.
4. Materiał wystarcza do przygotowania zgłoszenia potencjalnego błędu integracji Qt Multimedia: samodzielny kod, receptury wejścia, parametry środowiska, skończona macierz i dump. Zgłoszenie powinno wyraźnie oddzielać obserwacje od hipotez. Nie wysłano zewnętrznego zgłoszenia.

Do osobnego zadania kwalifikują się porównanie na kompletnym nowszym zestawie, inne urządzenie audio/sterownik, rzeczywisty build i testy Linux oraz uzyskanie pełnych symboli dla ustalenia przyczyny. Ręczny odsłuch i fizyczne suspend/resume Windows pozostają wymaganiami kwalifikacji wydania z poprzednich etapów. Brak tych danych ogranicza decyzję wydaniową; nie został przedstawiony jako zaliczenie testów.

## Zmienione pliki i zachowanie dowodów

Zmiany etapu 5D obejmują:

- `CMakeLists.txt` i `audio_diagnostics.pro`: zależność DbgHelp wyłącznie dla diagnostyki Windows.
- `main.cpp`: rozszerzenia samodzielnego reproduktora opisane powyżej.
- `prepare_fixtures.ps1` i `run_matrix.ps1`: przygotowanie danych i kontrolowane procesy oraz dopisane przy wznowieniu zabezpieczenia plików.
- `STAGE5D.md` i `stage5d-results.json`: raport i zestawienie wyników z sumami artefaktów.

Dotychczasowe raporty 5B/5C, kod produkcyjny i testy regresyjne zachowano. Nie zmieniono danych użytkownika, ulubionych ani istniejących nagrań. Nie wykonano commit, push, merge ani zmiany gałęzi.

Surowe pliki pozostają w `%TEMP%/TuneScape-stage5d-54fe4763`: `matrix/`, `fixtures/`, `qmake/`, `cmake/`, logi kompilacji, `full-qmake.txt`, `verification-ctest.log`, `dump-exceptions.json` i `crash-worker-4-lldb.txt`. Nie nadpisano ich i nie usuwano. Jest to katalog tymczasowy; repozytorium przechowuje raport, receptury i kompaktowy manifest, bez binariów narzędzi i dumpów. Binarne artefakty wymagają zachowania tego katalogu, jeśli mają później służyć do zgłoszenia.

Skrypty wymagają PowerShell 7, osobno wskazanych narzędzi i nowych pustych katalogów. Nie pobierają ani nie instalują Qt lub FFmpeg. `run_matrix.ps1` zwraca niezerowy kod, gdy choć jeden proces zakończy się błędem lub timeoutem. Pełna macierz 5D jest wynikiem negatywnym dla stabilności bieżącego toru mono MP2, pomimo poprawnego końcowego CTest.
