# TuneScape — Etap 5E: rzeczywiste porównanie Qt 6.7.0 i Qt 6.12.0

Raport z 18 listopada 2026. Gałąź `refactor/codebase-audit`, baza `50f51e3`.

**Wykonano pełne porównanie: po 144 niezależne procesy dla każdego zestawu. Qt 6.7.0 zakończył 10 prób awarią natywną; na Qt 6.12.0 nie zaobserwowano awarii w 144 próbach.** Nowy zestaw przeszedł również świeże buildy całej aplikacji qmake/CMake, pełny QtTest oraz CTest. Wyniki przemawiają za kontrolowaną migracją Windows do badanego zestawu 6.12.0, z ograniczeniami opisanymi poniżej. Konfiguracja produkcyjna nie została zmieniona.

Jest to wznowienie wcześniejszego przygotowania 5E. Pierwotny raport o braku zestawu 6.11.3 zachowano w katalogu dowodów jako `STAGE5E-before-6120.md`. Tamten etap nie zawierał pomiarów nowego Qt. Poniższe wyniki dotyczą rzeczywiście zainstalowanego Qt 6.12.0 i nowych przebiegów obu wersji, bez zastępowania ich historycznymi liczbami z 5D.

## 1. Zweryfikowane konfiguracje

| Składnik | Baza | Nowy zestaw |
| --- | --- | --- |
| Qt / Qt Multimedia, nagłówki i runtime | 6.7.0 / 6.7.0 | 6.12.0 / 6.12.0 |
| Qt6Multimedia.dll i załadowany plugin FFmpeg | 6.7.0.0 / 6.7.0.0 | 6.12.0.0 / 6.12.0.0 |
| GCC programu i zestawu Qt | 11.2.0 / 11.2.0 | 15.1.0 / 15.1.0 |
| Architektura | x86_64, LLP64 | x86_64, LLP64 |
| FFmpeg faktycznie w procesie | 6.1.1 | 9.0.1 |
| avcodec | 60.31.102, avcodec-60.dll | 63.1.101, avcodec-63.dll |
| avformat | 60.16.100, avformat-60.dll | 63.1.101, avformat-63.dll |
| avutil | 58.29.100, avutil-58.dll | 61.1.101, avutil-61.dll |
| swresample | 4.12.100, swresample-4.dll | 7.1.101, swresample-7.dll |
| Wyjście audio | Domyślne Realtek, stereo / 48000 Hz / Float | To samo urządzenie i format preferowany |

Ścieżki Qt to `C:/Qt/6.7.0/mingw_64` i `C:/Qt/6.12.0/mingw_64`. Stary kompilator znajduje się w `C:/Qt/Tools/mingw1120_64/bin`. Nowy użyty kompilator znajduje się w:

`C:/Users/gkedzior/AppData/Local/Temp/TuneScape-stage5e-6120-be58c7b1/mingw1510/bin`.

Ponowne sprawdzenie katalogów, dostępnych plików g++.exe/gcc.exe i metadanych instalacji wykazało zainstalowane GCC 11.2 i 13.1 oraz MSYS2 UCRT GCC 14.2. W podanym przez użytkownika `C:/Qt/6.12.0/mingw_64/bin` były qmake i biblioteki runtime MinGW, lecz brak g++.exe, gcc.exe i mingw32-make.exe. Nie użyto starszego kompilatora do nowego Qt.

Aby dokończyć zadanie bez niedziałającego instalatora, pobrano i rozpakowano wyłącznie do nowego katalogu tymczasowego oficjalny przenośny toolchain `qt.tools.win64_mingw1510`, wersja `15.1.0-202610271023`. GCC potwierdził 15.1.0, wariant x86_64-posix-seh-msvcrt-rt_v12-rev0. Wersja zgadza się z `QT_GCC_MAJOR/MINOR/PATCH_VERSION` z qconfig.pri zainstalowanego Qt. Istniejące instalacje Qt i kompilatorów nie zostały nadpisane; nie zmieniono globalnego PATH. Nie naprawiano samego instalatora, ponieważ nie był potrzebny do przeprowadzenia porównania.

Archiwum pochodziło z [oficjalnego repozytorium MinGW 15.1 Qt](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/tools_mingw1510/qt.tools.win64_mingw1510/15.1.0-202610271023mingw1510.7z). Sprawdzono zgodność z opublikowanym obok plikiem SHA-1: `C32AC26E95383ACF210CA9DB74D16171402DAE45`. Dodatkowo zapisano SHA-256: `23250E2BB68D8E9A54170975D0027A405175C7ED01B397FFE72E2E89BFE00ED3`.

### Kontrola ładowanych bibliotek

Każdy proces miał osobny PATH: bin wybranego Qt, bin właściwego kompilatora i katalogi systemowe Windows. QT_PLUGIN_PATH i QT_QPA_PLATFORM_PLUGIN_PATH wskazywały ten sam zestaw, QT_MEDIA_BACKEND=ffmpeg, QT_QPA_PLATFORM=offscreen. Środowisko zmieniano wyłącznie dla procesu testowego.

Inwentarz Windows odczytuje faktycznie załadowane moduły, a wersje FFmpeg przez ich eksportowane funkcje. Wersje kompilacji Qt/Qt Multimedia i GCC porównano z runtime oraz wybranym zestawem. Sprawdzono również wersje plików Qt Multimedia i pluginu.

| Moduł | Baza — rzeczywista ścieżka | Nowy zestaw — rzeczywista ścieżka |
| --- | --- | --- |
| Qt6Core.dll | C:/Qt/6.7.0/mingw_64/bin/Qt6Core.dll | C:/Qt/6.12.0/mingw_64/bin/Qt6Core.dll |
| Qt6Multimedia.dll | C:/Qt/6.7.0/mingw_64/bin/Qt6Multimedia.dll | C:/Qt/6.12.0/mingw_64/bin/Qt6Multimedia.dll |
| Plugin | C:/Qt/6.7.0/mingw_64/plugins/multimedia/ffmpegmediaplugin.dll | C:/Qt/6.12.0/mingw_64/plugins/multimedia/ffmpegmediaplugin.dll |
| Resampler | C:/Qt/6.7.0/mingw_64/bin/swresample-4.dll | C:/Qt/6.12.0/mingw_64/bin/swresample-7.dll |

Pozostałe ścieżki i sumy DLL są w logach, `kit.json` i `stage5e-comparison.json`. Kontrola wszystkich 288 logów nie wykazała ścieżek Qt/FFmpeg spoza właściwego zestawu. Zero wyników oznaczono jako nieporównywalne. Nie traktowano kompilacji nowymi nagłówkami i uruchomienia ze starym runtime jako poprawnego pomiaru.

## 2. Identyczne dane wejściowe i protokół

Wykorzystano istniejący reproduktor, `run_matrix.ps1` i próbki przygotowane przez `prepare_fixtures.ps1` w 5D. Nie kodowano ich ponownie. Skopiowano pliki do nowego katalogu dowodów i zweryfikowano SHA-256 manifestu. Oba buildy diagnostyczne powstały z identycznego źródła, którego sumy zapisano w `source-hashes.json`. Nie zmieniano kodu pomiędzy seriami.

| Próbka | SHA-256 |
| --- | --- |
| MP2 mono 44100 Hz | 965A12AC99DC09FFB471B7FB2373693D96210CDD27035573C4C9B7FD985D4D24 |
| MP2 mono 48000 Hz | 379264C9EC4A0CAD4252112961A30490C78663FEED45D72500FED4A40D7CD967 |
| MP2 stereo 44100 Hz | CA93BDCAE073F96D5D3BAA63B76E777E323454138F34FA3AEB27E646E3109130 |
| MP3 mono 44100 Hz | 8E73821017ED933DCC9C72A79AB0E170E347187EDDE32B6A43F516B3160B6E92 |
| MP3 stereo 44100 Hz | 350E72F61136FE180A9E08774D4DFC6167A5FFD626D18D733FFBEA7249E10036 |
| WAV PCM mono 44100 Hz | DB75D9957B06298EC6949BDD9B60297C08E20A82A292B78FBCA5352BCD1E9A3F |

Każda z sześciu próbek była testowana przez QMediaPlayer i QAudioDecoder, po 12 niezależnych procesów. Worker QMediaPlayer miał zadane 12 cykli, zmiany A/B, szybkie przełączenia oraz końcowe niszczenie aktywnego dekodera. QAudioDecoder wykonywał samodzielne dekodowanie pliku. Oba tryby korzystają z tych samych bajtów wejścia. Worker używa kontrolowanego lokalnego HTTP; dekoder odczytuje plik tymczasowy. Limit procesu wynosił 60 sekund.

Najpierw zakończono 144 próby 6.7.0, następnie uruchomiono 144 próby 6.12.0. Nie prowadzono obu serii odtwarzania równocześnie. Nie jest to losowany eksperyment wydajnościowy ani izolacja pojedynczej biblioteki; zmienił się cały zestaw, w tym kompilator i jego ustawienia. Wszystkie pliki wejściowe odczytane w logach zgadzają się z manifestem.

## 3. Pełna macierz porównawcza

Każdy wiersz zawiera po 12 osobnych procesów na wersję. Poprawny wynik wymagał kodu 0 oraz zgodności środowiska.

| Materiał | API | 6.7.0 poprawne | 6.7.0 awarie | 6.12.0 poprawne | 6.12.0 awarie |
| --- | --- | ---: | ---: | ---: | ---: |
| MP2 mono 44100 | QMediaPlayer | 11 | 1 | 12 | 0 |
| MP2 mono 44100 | QAudioDecoder | 10 | 2 | 12 | 0 |
| MP2 mono 48000 | QMediaPlayer | 7 | 5 | 12 | 0 |
| MP2 mono 48000 | QAudioDecoder | 10 | 2 | 12 | 0 |
| MP2 stereo 44100 | QMediaPlayer | 12 | 0 | 12 | 0 |
| MP2 stereo 44100 | QAudioDecoder | 12 | 0 | 12 | 0 |
| MP3 mono 44100 | QMediaPlayer | 12 | 0 | 12 | 0 |
| MP3 mono 44100 | QAudioDecoder | 12 | 0 | 12 | 0 |
| MP3 stereo 44100 | QMediaPlayer | 12 | 0 | 12 | 0 |
| MP3 stereo 44100 | QAudioDecoder | 12 | 0 | 12 | 0 |
| WAV mono 44100 | QMediaPlayer | 12 | 0 | 12 | 0 |
| WAV mono 44100 | QAudioDecoder | 12 | 0 | 12 | 0 |
| **Razem** | | **134** | **10** | **144** | **0** |

| Kategoria | Qt 6.7.0 | Qt 6.12.0 |
| --- | ---: | ---: |
| Procesy | 144 | 144 |
| Sukces | 134 | 144 |
| Awaria natywna, kod 0xC0000005 | 10 | 0 |
| Timeout | 0 | 0 |
| Nieobsługiwany format | 0 | 0 |
| Inny błąd procesu | 0 | 0 |
| Nieporównywalne / błąd konfiguracji | 0 | 0 |
| Zachowane minidumpy | 10 | 0 — nie wystąpiła awaria |

Kod skryptu bazowego wyniósł 1, ponieważ prawidłowo zgłosił awarie; nie jest to nieukończona seria. Skrypt nowego zestawu zwrócił 0. Są to nowe wyniki 5E. Nie należy dodawać do nich historycznych 192 prób/22 awarii z 5D ani wcześniejszych pojedynczych uruchomień diagnostyki.

## 4. Dekodery, przełączanie i zamykanie

W udanych dekodowaniach obu zestawów uzyskano te same parametry i liczbę ramek PCM:

| Materiał | Kanały / Hz | Format PCM | Ramki |
| --- | --- | --- | ---: |
| MP2 mono 44100 | 1 / 44100 | Float | 264960 |
| MP2 mono 48000 | 1 / 48000 | Float | 288000 |
| MP2 stereo 44100 | 2 / 44100 | Float | 264960 |
| MP3 mono 44100 | 1 / 44100 | Float | 266112 |
| MP3 stereo 44100 | 2 / 44100 | Float | 266112 |
| WAV mono 44100 | 1 / 44100 | Int16 | 264600 |

Nie stwierdzono utraty obsługi któregoś wymaganego formatu. Nie porównywano PCM próbka po próbce i nie wykonywano subiektywnego odsłuchu; zgodność liczników nie dowodzi identyczności sygnału.

Nowy zestaw zaliczył 72 procesy workerów, łącznie 864 zadane cykle i 72 końcowe zamknięcia aktywnego dekodera. Po udanych cyklach wymagano usunięcia starych QMediaPlayer i zamknięcia połączeń. Wszystkie końcowe rekordy udanych workerów obu wersji wykazują zero pozostawionych odtwarzaczy, zero aktywnych połączeń HTTP i zero zaobserwowanych callbacków poprzedniej próby. Współdzielone QAudioOutput pozostaje żywe do końca workera i jest niszczone po właścicielu odtwarzaczy. Awarie bazowych procesów nie są zaliczeniem zamknięcia sesji.

### Dowód awarii bazowej

Każdemu z 10 niepoprawnych procesów bazowych odpowiada niepusty minidump. LLDB z `C:/Qt/Tools/QtCreator/bin/clang/bin/lldb.exe` otworzył `encoded-mono-mp2-44100-decode-none-10.dmp`. Zapisano `thread backtrace all` w `qt670-backtrace.txt`.

Wątek awarii ma wyjątek 0xC0000005, ramki w swresample-4.dll, swr_convert() oraz ffmpegmediaplugin.dll. Nazwy eksportów pluginu z dużymi przesunięciami nie są wiarygodnymi nazwami wewnętrznych funkcji; brak pełnych symboli ogranicza dokładność interpretacji. Nie analizowano instrukcji ani rejestrów w celu szukania następnej sekwencji wywołującej awarię.

To wskazuje ścieżkę zaobserwowanej awarii, nie dowodzi samodzielnego błędu FFmpeg. Porównanie nie rozdziela wpływu Qt Multimedia, FFmpeg, GCC, zarządzania buforami i czasu życia obiektów. Awaria samodzielnego reproduktora nie wymaga GUI ani PlaybackController TuneScape, lecz nie dowodzi, że każda możliwa awaria aplikacji ma ten sam powód. Na 6.12.0 nie powstał dump, ponieważ nie zaobserwowano awarii.

## 5. Integracja TuneScape na Qt 6.12.0

Integrację rozpoczęto dopiero po poprawnym zakończeniu całej nowej macierzy. Użyto osobnych świeżych katalogów, nagłówków i bibliotek Qt 6.12.0 oraz tego samego GCC 15.1.0. Nie zmieniono plików projektu w celu wyboru nowego Qt.

| Kontrola | Wynik |
| --- | --- |
| Świeży build qmake, aplikacja + testy + diagnostyka | Kod 0 |
| Pełny QtTest qmake, TUNESCAPE_TEST_MEDIA=1 | **84 zaliczenia, 0 błędów, 1 jawne pominięcie**, 45,462 s |
| Świeży CMake RelWithDebInfo, aplikacja + testy + diagnostyka | Kod 0 |
| Pełny CTest, TUNESCAPE_TEST_MEDIA=1 | **2/2**, 84,43 s; główny zestaw 49,26 s, diagnostyka 35,14 s |
| Play, A → B → A, 6 Pause/Resume, dwukrotny Stop | Zaliczony istniejący test rzeczywistego backendu |
| Szybkie przełączenie z oczekującego HTTP | Zaliczony |
| Niszczenie aktywnego backendu, player przed output | Zaliczony |
| Nagrywanie podczas Play i Pause | Zaliczony; zapis rośnie podczas Pause |
| Stop i zmiana stacji podczas nagrywania | Zaliczony; zachowane dokładne wcześniej zapisane bajty |
| Zwalnianie HTTP i odtwarzaczy | Zaliczony; oczekiwane zero po Stop |
| Ponowny odczyt nagrań | Zaliczony przez rzeczywisty backend |
| HTTP 404/503, timeout, przerwana odpowiedź i odzyskanie | Zaliczony |
| Pozostałe lokalne regresje, m.in. Favorites/M3U, ikony, P0 Stop | Objęte pełnym zaliczonym QtTest |

Szczegółowym dowodem są testMediaHttpSwitchAndRelease, testMediaHttpRecordingLifecycle i testMediaHttpErrorsAndRecovery oraz ich logi procesów potomnych. Nie pomijano problematycznych natywnych prób, aby uzyskać dodatni wynik. Pominięto wyłącznie test publicznych stacji, ponieważ nie podano TUNESCAPE_LIVE_STREAM_A/B. Internetowe odtwarzanie i nagrywanie, w tym SomaFM, nie zostały sprawdzone ponownie w tym wznowieniu.

Nie wykryto regresji API ani funkcjonalnej w objętym zakresie. GCC 15 zgłosił ostrzeżenia o konwersji FARPROC w inwentarzu diagnostycznym oraz o nieużywanym parametrze. Nie były to błędy kompilacji ani nowa poprawka produkcyjna. Nie zmieniano kodu odtwarzania lub rejestratora w odpowiedzi na te ostrzeżenia.

Logi zawierają też ostrzeżenia trybu offscreen o katalogu fontów oraz o stylu mini-odtwarzacza, obecne również w wynikach starszego środowiska. Pełny QtTest nie jest wizualną oceną wyglądu aplikacji uruchomionej na pulpicie; tę kontrolę należy zachować w kwalifikacji wydania.

## 6. Ocena i decyzja wydaniowa

**Qt 6.12.0 / GCC 15.1.0 / FFmpeg 9.0.1 jest lepszym zaobserwowanym wyborem Windows spośród dwóch sprawdzonych konfiguracji.** Awaria MP2 mono powtórzyła się na bazie, a nie została zaobserwowana na nowym kompletnym zestawie przy identycznych plikach i źródle reproduktora. Pozytywna integracja przemawia za kontrolowaną migracją.

Nie stwierdzono matematycznej pewności stabilności ani przyczyny na poziomie konkretnej poprawki biblioteki. Po 12 procesów na wariant to ograniczona seria. Krótkie cykle i osobne procesy nie zastępują testu wielogodzinnego działania jednego procesu ani pełnego profilowania pamięci. Nie badano innych urządzeń, sterowników, architektur i systemów.

Zalecana kolejność dalszych działań:

1. Utrwalić kompletny zestaw Qt 6.12.0 / MinGW 15.1.0 i wersje FFmpeg w odtwarzalnym środowisku budowania. Obecny przenośny kompilator jest w katalogu tymczasowym; jego trwała instalacja lub zachowanie archiwum jest osobnym krokiem. Nie podmieniać pojedynczych DLL.
2. W osobnej zmianie aktualizować konfigurację budowania/CI Windows, pozostawiając qmake jako system podstawowy i zachowując możliwość odtworzenia starej bazy. W tym zadaniu nie zmieniono konfiguracji produkcyjnej ani workflow.
3. Zweryfikować deployment narzędziem windeployqt z nowego zestawu, runtime GCC 15.1, pluginy i FFmpeg na maszynie bez developerskiego PATH. Sam build i testy z instalacji SDK nie zaliczają paczki dystrybucyjnej.
4. Przeprowadzić natywny build i testy Linux z jego zgodnym Qt/FFmpeg. Nie przenosić bibliotek Windows do Linux i nie uznawać wyników Windows za weryfikację Linux.
5. Przed stabilnym wydaniem wykonać odsłuch, dłuższe sesje/przełączanie, publiczne strumienie i fizyczne suspend/resume Windows. W razie błędu zachować log i dump; nie dopasowywać synchronizacji przypadkowymi opóźnieniami.

Migracja Windows jest uzasadniona pomiarami jako kolejny kontrolowany krok. Automatyczne przestawienie produkcji i bezwarunkowa deklaracja gotowości całego wydania nie są uzasadnione bez powyższej kwalifikacji. Stwierdzenie wyniku brzmi: **nie zaobserwowano awarii na 6.12.0 w wykonanej serii**, a nie: usunięto wszystkie możliwe awarie MP2 mono.

## 7. Dowody, odtwarzanie i zakres zmian

Wszystkie nowe artefakty są w:

`C:/Users/gkedzior/AppData/Local/Temp/TuneScape-stage5e-6120-be58c7b1`.

Najważniejsze pliki i katalogi:

- `fixtures/`: kopie plików 5D i manifest, bez ponownego kodowania.
- `qt670-diagnostics/`, `qt6120-diagnostics/`: osobne buildy qmake tego samego reproduktora.
- `matrix-670/`, `matrix-6120/`: po 144 logi, kody procesów w matrix.json, metryki zgodności i kit.json; 10 minidumpów w pierwszym katalogu.
- `qt670-backtrace.txt`: odczyt jednego dumpa przez LLDB.
- `mingw1510/`, archiwum i sumy, `qt670-qconfig.pri`, `qt6120-qconfig.pri`, `source-hashes.json`: pochodzenie narzędzi i źródła.
- `qt6120-qmake-app/`, `qt6120-cmake-app/`: świeże buildy całej aplikacji.
- `qt6120-full-qmake.txt`, `qt6120-ctest.log`, logi CMake i `integration-status.json`: wyniki integracji.
- `STAGE5E-before-6120.md`: kopia wcześniejszego raportu przygotowania.

Kompaktowy trwały wynik znajduje się w repozytorium w `stage5e-comparison.json`: obie konfiguracje, metryki dekoderów, wszystkie komórki macierzy, sumy próbek/DLL/wybranych artefaktów i status integracji. Surowych dumpów i narzędzi nie dodawano do repozytorium. Katalog dowodów jest tymczasowy i należy zachować go wraz z archiwum kompilatora, jeżeli ma służyć późniejszemu zgłoszeniu lub odtworzeniu.

Aby powtórzyć macierz, w PowerShell 7 wywołaj istniejący run_matrix.ps1 osobno dla każdego powyższego EXE, jego QtRoot i CompilerBin. Wskaż ten sam fixtures/, nowe puste ResultsDirectory, sześć nazw encoded-mono-mp2-44100, encoded-mono-mp2-48000, encoded-stereo-mp2, encoded-mono-mp3, encoded-stereo-mp3, encoded-mono-pcm, `-Trials 12 -Cycles 12 -CaptureDump`. Worker i decode są trybami domyślnymi. Nie uruchamiaj obu serii odtwarzania jednocześnie i nie używaj trybu --resample z ABI FFmpeg 6 do oceny FFmpeg 9.

Przy pełnym buildzie użyto qmake z Qt 6.12.0 i mingw32-make z przenośnego 15.1. Dla CMake wskazano jawnie `CMAKE_PREFIX_PATH` i `Qt6_DIR` zestawu 6.12.0, `CMAKE_CXX_COMPILER` z tego samego GCC 15.1, generator Ninja i świeży katalog. Testy lokalne miały TUNESCAPE_TEST_MEDIA=1 i offscreen; zmienne publicznych strumieni były usunięte ze środowiska tych procesów.

W tym wznowieniu nie zmieniano źródeł reproduktora pomiędzy buildami ani skryptu macierzy. Zachowano wcześniejsze rozszerzenia metryk 5E w main.cpp i run_matrix.ps1, zaktualizowano STAGE5E.md i dodano stage5e-comparison.json. PlaybackController, RadioAudioManager, StreamRecorder, Favorites, M3U, Radio Browser, istniejące regresje i pliki konfiguracji produkcyjnej pozostały bez zmian. Nie naruszono danych użytkownika ani istniejących nagrań. Nie wykonano commit, push, merge ani zmiany gałęzi. Wcześniejsze wyniki 5C, 5D i przygotowania 5E zachowano.

**Kryterium etapu 5E zostało spełnione przez rzeczywiste porównanie obu kompletnych konfiguracji i rekomendację opartą na pomiarach.**
