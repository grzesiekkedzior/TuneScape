# Etap 6B — MusicBrainz, ICY i metadane

Baza: `20ba3071106323ccb71f09279d343329417005a7`, gałąź `refactor/codebase-audit`. Data: 2026-11-20. Początkowe drzewo czyste.

**Status: zakres zmian 6B ukończony i zweryfikowany lokalnie na Windows. Końcowy QtTest z audio oraz CTest PASS. Linux/CI dla zmian 6B oczekują na rzeczywisty workflow.** Nie deklaruje się wykonania builda Linux.

## Audyt przed zmianami

| Komponent | Odpowiedzialność / zależności | Potwierdzony problem | Poprawka / ryzyko |
| --- | --- | --- | --- |
| PlaybackController | Stacja, generacja, Pause/Resume/Stop | Generacja sesji i brak jej zmiany przy Pause już działają | Zachować, bez zmian |
| StreamReader → RadioList | Osobny HTTP z Icy-MetaData; generacja sesji | Brak ramkowania icy-metaint, dekodowanie audio jako UTF-8, utrata podzielonych bloków | Parser ograniczonego bloku ICY; ryzyko nietypowych stacji |
| RadioList / PlayerUIController / Tray | Deduplikacja StreamTitle i prezentacja | Tożsamość sesji już sprawdzana; Tray ignorował pusty tytuł przed odświeżeniem panelu | Zachować sygnał, wydzielać prawidłowe bloki i odświeżać także pusty tytuł |
| MusicBrainzClient | Search i lookup | int przechowuje czas epoch ms; limit gubi zapytania, nie obejmuje lookup; brak timeout/cache/kontekstu | Jedna kolejka wszystkich żądań MusicBrainz, token utworu, dosłowne zapytania Lucene; ryzyko doboru wyniku |
| ExternalLinksClient | Relacje artysty przez osobny manager | Obchodzi wspólny limit MusicBrainz, brak timeoutu/kontekstu | Korzystać ze wspólnej kolejki MusicBrainz |
| CoverArtClient | JSON Cover Art Archive | Brak timeout/status/limitu rozmiaru/kontekstu | Wspólny mały transport JSON, osobna kolejka CAA |
| ResolverService | Łączenie trzech wyników | Wspólny PendingTrack bez tokenu; puste/błędne wyniki potrafią blokować zakończenie; pierwszy wynik bez sprawdzenia zgodności | Token utworu, anulowanie, wybór zgodnego wyniku, zachowanie surowych danych przy błędzie |
| TrackView | Teksty i obraz okładki | Własny manager obrazów zamiast IconLoader; surowe wskaźniki UI, brak destruktora anulującego, HTML bez escaping | Współdzielony IconLoader, identyfikator obrazu i QPointer kontrolek; bez redesignu |
| MainWindow | Składanie komponentów | Anulowanie przy sessionChanged już jest, ale nowy utwór nie czyści od razu starej okładki | Powiązać czyszczenie widoku z nową generacją utworu |

Przed: Reader → surowe porcje → wyszukiwanie tekstu w RadioList → resolver → trzy niezależne klienty → TrackView → osobny HTTP obrazu. Po: ramka ICY + generacja sesji → deduplikowany tytuł → token utworu → ograniczone kolejki/cache JSON (jedna wspólna dla MusicBrainz) → TrackView → istniejący IconLoader.

Nie przebudowano kontrolera odtwarzania, RadioList/Country ani rejestratora. Tabela powstała przed implementacją, później uzupełniono ją o potwierdzony błąd pustego tytułu w Tray.

## Wprowadzone poprawki

### Sesja i utwór

Zachowano istniejącą generację PlaybackController i kontrolę tej generacji w RadioList. Nie zmieniono PlaybackController ani RadioAudioManager. Zmiana sesji nadal anuluje resolver; Pause/Resume nie zmienia sesji i nie kasuje bieżących danych. Osobny Reader nadal obserwuje strumień podczas Pause, zgodnie z dotychczasowym działaniem.

Resolver otrzymał monotoniczny token utworu. Zwiększa go przy nowym utworze i cancel/Stop; wszystkie odpowiedzi klientów niosą ten token. Sloty sprawdzają także etap Search/Lookup/Enrich, więc stara lub powtórzona odpowiedź nie uzupełnia nowego Track. Kolejki mają dodatkową generację anulowania obejmującą aktywną odpowiedź, oczekujące zadania oraz opóźnione callbacki cache. Zmiana A → B → A nie przywraca starego tokenu A.

Nowy utwór natychmiast czyści poprzednią okładkę/album/linki i prezentuje poprawny tekst ze strumienia. Dane MusicBrainz są uzupełnieniem, nie warunkiem pokazania tytułu. Tytuł bez wykonawcy jest poprawnym Track. Brak wyniku lub błąd HTTP pozostawia dane ICY; puste/nieprawidłowe metadane oraz Stop czyszczą szczegóły. Sygnał cleared resolvera obsługuje TrackView; zamknięcie głównego widgetu również anuluje żądania.

### ICY

StreamReader interpretuje dodatni nagłówek `icy-metaint`, pomija odcinki audio, odczytuje bajt długości i składa cały blok metadanych w jednostkach 16 bajtów. Przechowuje maksymalnie 4080 bajtów bloku, a audio czyta porcjami. Stan parsera zeruje się przy każdej zmianie odpowiedzi/sesji. Fragmentacja HTTP i rozcięcie wewnątrz znaku UTF-8 nie gubią tekstu.

Brak/nieprawidłowy metaint oznacza brak zaufanych metadanych; nie wyszukuje się StreamTitle w losowych bajtach audio. Blok o zerowej długości oznacza brak aktualizacji. Jawne `StreamTitle='';` oznacza wyczyszczenie danych. Niepełny blok na końcu i pole uszkodzone składniowo są pomijane. Tekst dekoduje się jako UTF-8, z fallbackiem Latin-1 przy niepoprawnym UTF-8 (historyczne stacje); bez deklaracji kodowania nie da się zawsze odgadnąć intencji nadawcy.

RadioList zachowuje deduplikację i jedynie wyodrębnia kompletne pole StreamTitle. Dodano normalizację Unicode NFC. Nie zmieniono wyboru stacji ani obsługi odtwarzania/nagrywania. Dwie stare odpowiedzi syntetyczne w teście zmiany stacji zastąpiono prawdziwymi ramkami ICY; zachowano wszystkie asercje sesji, metadanych i zapisanych bajtów nagrania.

### MusicBrainz, CAA i normalizacja

Dodano jeden mały wspólny transport JsonRequestQueue, ponieważ trzy istniejące klienty powielały niekompletną obsługę HTTP. MusicBrainzClient i ExternalLinksClient korzystają z tej samej instancji: jedna aktywna odpowiedź i minimum 1000 ms pomiędzy startami search, lookup i relations, również po anulowaniu. Nie ma już zapytań porzucanych tylko dlatego, że przyszły przed upływem sekundy. CAA używa osobnej instancji z jedną aktywną odpowiedzią.

Wszystkie żądania JSON mają timeout 8 s, limit odpowiedzi 1 MiB, kontrolę HTTP/JSON, spójny User-Agent TuneScape/6.0 z adresem projektu i kontaktem, bez automatycznych agresywnych retry. 429/503 blokują kolejkę co najmniej na 30 s, z uwzględnieniem Retry-After (sekundy lub data, do 24 h). Zmiana stacji nie zeruje zegara limitu ani backoffu. HTTP 404/5xx, wadliwy JSON i timeout kończą odpowiednią operację; opcjonalny brak okładki/linków nie blokuje Track.

Zgodnie z [MusicBrainz API](https://musicbrainz.org/doc/MusicBrainz_API) aplikacja ma nie przekraczać jednego wywołania na sekundę i identyfikować się User-Agent. Parametry search są dosłownymi frazami pól Lucene: escape cudzysłowów/backslash oraz osobne kodowanie URL, w tym `%2B`. Zachowuje się Unicode, AC/DC, współwykonawców i wersję utworu, zamiast usuwać arbitralnie fragmenty tytułu. Konserwatywny separator to spacja-myślnik-spacja oraz jego odmiany Unicode. Pozostały tekst może być tytułem bez wykonawcy; jednoznaczne URL, puste/uszkodzone dane i proste komunikaty reklamowe nie uruchamiają HTTP.

Resolver wybiera wyłącznie zgodny z tytułem i, jeśli znany, wykonawcą rekord z prawidłowym MBID. Przy kilku zgodnych, różnych nagraniach nie zgaduje — pozostawia ICY. Lookup jest ponownie sprawdzany względem ID i tytułu/wykonawcy. CAA: preferowana grafika front i miniatura 250 px, zgodnie z [API Cover Art Archive](https://musicbrainz.org/doc/Cover_Art_Archive/API), z fallbackiem do pola image. Linki i obraz wymagają schematu HTTP/HTTPS. Wewnętrzne sygnały klientów rozszerzono o token; wywołania metod zachowują domyślny token dla istniejących wywołań bez tego parametru.

QueryData przeniesiono z anonimowej przestrzeni nazw w nagłówku do music_data i dodano jawny include QString: typ używany pomiędzy jednostkami translacji musi mieć wspólną tożsamość. Nie zmieniono modelu stacji, Favorites/M3U ani Recorder.

### Okładki i widok

MainWindow przekazuje TrackView istniejący współdzielony IconLoader. Nie zmieniono implementacji ani cache IconLoader. Usunięto osobny manager sieciowy okładek, nieograniczone readAll/dekodowanie QPixmap w TrackView i surowy przechowywany Ui pointer. Widok ma QPointer do konkretnych kontrolek, token obrazu, kontrolę request ID, anulowanie przy clear/destruktorze i zniszczeniu widgetu. Rozmiar żądania obrazu 250×250 uwzględnia DPR; limity pobierania/dekodowania oraz cache zapewnia IconLoader.

Teksty metadanych w MainWindow, MiniPlayer i Tray mają PlainText. Dynamiczne etykiety/linki są escapowane przy generowaniu HTML; URL spoza HTTP/HTTPS jest odrzucany. Poprawiono też Tray: odświeżenie panelu następuje przed pominięciem powiadomienia o pustym tytule. Jawne puste ICY pokazuje Live stream, a Stop Select a station, zamiast poprzedniego utworu. Nie zmieniono układu, motywów ani ikon zastępczych.

## Liczba żądań, cache i GUI

| Scenariusz | Przed: analiza kodu, bez benchmarku starej wersji | Po: pomiar lokalnych testów |
| --- | --- | --- |
| Nowy, rozpoznany utwór, album i wykonawca dostępne | Do 3 MusicBrainz + 1 CAA JSON + 1 obraz = 5 GET | 4 JSON GET; obraz 1 GET osobno w teście widoku; koszt zimnego odczytu zasadniczo ten sam |
| Dziesięć identycznych wejść | RadioList już deduplikował pełne identyczne tytuły; resolver sam nie miał deduplikacji/cache | Jeden proces rozpoznawania, brak dodatkowych GET i aktualizacji widoku |
| Powrót do już rozpoznanego utworu | Brak cache JSON i obrazu w TrackView, ponowne pobrania do 5 GET | 0 nowych GET JSON w teście resolvera, 0 nowych GET obrazu w teście widoku |
| Zmiana utworu podczas pobierania CAA i powrót | Brak tokenów wyników, anulowanie przez przeszukiwanie potomków | Stara odpowiedź odrzucona; jeden nowy GET anulowanej wcześniej okładki JSON, pozostałe 3 JSON z cache |
| Błąd/pusty wynik | Brak cache wyników negatywnych | Kolejne to samo żądanie z cache, bez HTTP |

Każda z dwóch kolejek JSON ma QCache o koszcie maksymalnie **2 MiB** (bajty odpowiedzi + URL + narzut 512), maksymalnie 32 różne oczekujące/aktywne URL i jedną aktywną odpowiedź do 1 MiB. Budżet cache JSON razem to około **4 MiB kosztu**, nie pomiar dokładnego RSS struktur Qt. Identyczne równoczesne URL mają jeden GET i wielu odbiorców. Pozytywne JSON są przechowywane 10 min, HTTP 404 5 min, inne błędy 30 s lub dłuższy Retry-After; poprawny JSON z pustą kolekcją też jest cache'owany. Cache jest tylko RAM, bez zapisu danych użytkownika. IconLoader zachowuje istniejący współdzielony limit 16 MiB; po zmianie służy także okładkom albumów.

Test limitu zapełnia kolejkę 12 rzeczywistymi odpowiedziami po 256 KiB: ostatnia jest ponownie użyta, pierwsza pobrana ponownie po usunięciu z cache. Przerwanie usuwa aktywną odpowiedź i zadania bez używania stałych tymczasowych plików.

GUI: jeden sygnał tytułu na rzeczywistą zmianę kompletnego StreamTitle. Resolver emituje cleared przy nowej generacji, następnie najwyżej surowy Track oraz jeden Track uzupełniony. Nieprawidłowy/niejednoznaczny wynik nie dopisuje przypadkowego albumu. Powtórzenia nie uruchamiają tego cyklu. MusicBrainz i CAA mogą pracować jednocześnie (najwyżej 2 odpowiedzi JSON), obrazy nadal podlegają współdzielonemu schedulerowi IconLoader. Strumień metadanych ICY pozostaje osobnym HTTP, tak jak przed zmianą.

## Regresje

Dodano **20 przypadków QtTest** (10 funkcji, w tym dane):

- testMetadataNormalization: Unicode, tytuł bez artysty, interpunkcja, nieusuwanie wersji Live, puste i uszkodzone dane.
- testMetadataHttpFailures: 404, 429 z backoff, 500, błędny JSON, timeout, przekroczenie limitu odpowiedzi; deduplikacja 2 równoczesnych żądań i cache negatywny.
- testMetadataQueuePacingAndCancellation: odstęp startów, anulowanie oczekujących operacji i opóźnionego cache callbacku, niszczenie transportu przy aktywnym HTTP.
- testMetadataCacheBudget: rzeczywista treść 256 KiB, trafienie i usunięcie najstarszego wpisu.
- testMetadataEmptyResults: tytuł bez wykonawcy, pusty wynik, brak kolejnych GET, reklama/URL/puste wejście, niejednoznaczne nagrania i zamknięcie UI podczas HTTP.
- testMetadataResolverIdentityAndCache: wybranie zgodnego rekordu zamiast pierwszego, nowy utwór podczas CAA, spóźnione lookup/cover ze starym tokenem, cache po powrocie, miniatura front.
- testMetadataSessionCancellation: A → B → A, przerwanie oczekującego HTTP, Pause/Resume bez restartu metadanych, Stop, niszczenie resolvera, kodowanie +/&/cudzysłowów i User-Agent.
- testMetadataCoverAndViewLifetime: zmiana utworu przy pobieraniu obrazu, nowa okładka, cache, clear, zniszczenie widgetów przed TrackView, tekst jako PlainText i filtr linków.
- testIcyFramingAndPresentation: fragmentacja także wewnątrz UTF-8, powtórzenia, uszkodzony/zerowy/pusty blok, MainWindow/MiniPlayer/Tray, Pause/Resume oraz Stop.
- testIcyMissingAndPartialBlocks: brak metaint i urwany blok nie produkują fałszywych metadanych.

Testy używają localhost, kontrolowanych odpowiedzi i istniejącego QTemporaryDir całego zestawu. Nie odpytywały publicznego API MusicBrainz/CAA ani stacji. Zachowano P0 Stop, PlaybackController, nagrywanie, Radio Browser, ikony i testy GUI; test synchronizacji Tray nadal sprawdza stan wspólnego odtwarzacza.

## Weryfikacja

Windows x64: Qt/Multimedia 6.12.0 z C:\Qt\6.12.0\mingw_64, GCC 15.1.0 z C:\Qt\Tools\mingw1510_64\bin, CMake/Ninja, C++17. Log rzeczywistego backendu potwierdza FFmpeg 9.0.1. Ścieżki runtime ustawiono jawnie na ten sam zestaw Qt/kompilatora. Testy używają QT_QPA_PLATFORM=offscreen.

| Kontrola | Wynik |
| --- | --- |
| Świeży qmake Release aplikacji, QtTest i audio_diagnostics | PASS; końcowe zmiany również skompilowane i zlinkowane |
| Pełny końcowy QtTest, TUNESCAPE_TEST_MEDIA=1 | **114 PASS, 0 FAIL, 1 SKIP**, 50,777 s, exit 0 |
| Poprzedni pełny QtTest z audio, przed poprawką pustego Tray | 114/0/1, 50,985 s |
| Świeży CMake/Ninja aplikacji i obu targetów testowych | PASS; końcowe zmiany również skompilowane |
| Końcowy CTest, TUNESCAPE_TEST_MEDIA=0 | **2/2 PASS**, 29,05 s; QtTest exit 0 (28,67 s), audio_diagnostics 2 PASS / 6 SKIP (opt-in audio) |
| Poprzedni CTest przed poprawką pustego Tray | 2/2 PASS, 29,42 s |
| Manifest źródeł, zgodność wielkości liter ścieżek | PASS; JsonRequestQueue.h/.cpp dodano do wspólnego sources.list używanego przez qmake/CMake |
| git diff --check | PASS |
| Linux / GitHub Actions dla zmian 6B | Niewykonane — brak lokalnego środowiska i brak commit/push |

Historyczny wynik wejściowy 94/0/1 obejmował 3 testy audio, wyłączone w trybie domyślnym. Po pierwszej części zmian stary zestaw dał 91/0/4. Rozszerzony zestaw domyślny dał 111/0/4 w 27,574 s, przed ostatnim doprecyzowaniem obsługi niejednoznacznych wyników. Przyrost o 20 dotyczy nowych przypadków, nie usunięcia testów. Pominięcie w trybie media dotyczy publicznych stacji wymagających osobnych zmiennych środowiskowych. Testy audio lokalnego HTTP, nagrywania, przełączania i odzyskiwania wykonały się w osobnych procesach. CTest używał trybu domyślnego; nie przedstawia się jego sześciu pominiętych przypadków osobnego audio_diagnostics jako wykonanych testów audio. Licznik 114/0/1 pochodzi z jawnego logu qmake QtTest; CTest potwierdza kod 0 drugiego builda (jego log nie zawiera tekstowego podsumowania TuneScapeTests).

W pośrednim buildzie ujawnił się problem anonimowego QueryData w nagłówku; poprawiono jego tożsamość pomiędzy jednostkami. Pierwszy nowy test cache wymagał doprecyzowania, że odpowiedź relations ma być odebrana przed anulowaniem oczekującej CAA — samo wysłanie odpowiedzi przez serwer nie jest zapisem do cache. Nie zmieniano aplikacji dla ukrycia tej różnicy. Późniejszy przegląd potwierdził niezależny błąd pustego tytułu w Tray, poprawiony wraz z regresją. Zachowano pośrednie logi; finalne wyniki powyżej dotyczą poprawionego kodu.

Pliki build/log poza repozytorium: `C:\Users\gkedzior\AppData\Local\Temp\TuneScape-stage6b-20261120`.

Dowody końcowe: `verified-qmake-build.log`, `verified-full-media.txt`, `cmake-configure.log`, `cmake-build.log`, `verified-cmake-build.log`, `verified-ctest-default.txt`, `cmake/Testing/Temporary/LastTest.log`. Dowody wcześniejszych kroków: `phase1-tests.txt`, `metadata-focused.txt`, `full-default.txt`, `full-final-media.txt`, `ctest-final-default.txt`. Nie nadpisano wyników wcześniejszych etapów.

## Zmienione pliki

- `TuneScapeCore/include/music_data/clients/JsonRequestQueue.h` i `TuneScapeCore/src/music_data/clients/JsonRequestQueue.cpp` — nowy wspólny transport, cache, limit, timeout i anulowanie.
- Pary nagłówek/źródło `MusicBrainzClient`, `CoverArtClient`, `ExternalLinksClient` w `TuneScapeCore/include/music_data/clients/` oraz `TuneScapeCore/src/music_data/clients/` — tokeny, URL, wspólna kolejka.
- Para `ResolverService.h/.cpp` w include/src/music_data/services — cykl życia utworu i dopasowanie wyników.
- `TuneScapeCore/include/music_data/models/QueryData.h`, `Track.h` — wspólny typ oraz tytuł bez wykonawcy.
- `TuneScapeCore/include/streamreader.h`, `TuneScapeCore/src/streamreader.cpp` — ramkowanie ICY.
- `TuneScapeCore/include/view/TrackView.h`, `TuneScapeCore/src/view/TrackView.cpp` — współdzielony IconLoader i bezpieczne kontrolki.
- `TuneScapeCore/src/mainwindow.cpp` — składanie powyższych komponentów i czyszczenie przy nowym utworze.
- `TuneScapeCore/src/radiolist.cpp` — tylko wyodrębnienie kompletnego pola ICY, normalizacja i strażnik życia widoku.
- `TuneScapeCore/src/controller/PlayerUIController.cpp`, `TuneScapeCore/src/trayicon.cpp` — PlainText i prawidłowe odświeżenie pustych danych w Tray.
- `TuneScapeCore/sources.list`, `TuneScapeTests/tst_tunescapetests.cpp`, `STAGE6B.md` — build, regresje i raport.

Nie zmieniono PlaybackController, RadioAudioManager, Country, StreamRecorder, RadioBrowserApi, Favorites/M3U, IconLoader, workflow, deploymentu ani motywów. Nie wykonano commit, push, merge ani zmiany gałęzi. Dane użytkownika i istniejące nagrania nie były używane ani modyfikowane.

## Ograniczenia i dalsze prace

- W tym środowisku WSL nie ma zainstalowanych dystrybucji (`WSL_E_DEFAULT_DISTRO_NOT_FOUND`), nie znaleziono Docker/Podman. Build Linux 6B nie został wykonany; nowe źródła wymagają rzeczywistego workflow. Wynik bazowego CI nie jest wynikiem zmian 6B. Nie wykonano commit/push/rerun starego HEAD.
- Źródła pozostają C++17 i używają API dostępnego w Qt 6.4, w tym QStringDecoder. Nie dodano przeciążeń QDebug ze std::string, chrono ani optional z nowszego Qt. Manifest ścieżek i ich wielkość liter sprawdzono; nowy transport jest w tym samym manifestcie qmake/CMake.
- Rozpoznawanie na podstawie tekstu nie jest identyfikacją akustyczną. Dokładne dopasowanie i odrzucenie niejednoznaczności może ograniczyć liczbę wzbogaconych utworów. Brak wykonawcy, błędne ICY i różnice pisowni pozostawiają tekst strumienia; nie dodano heurystyk zgadujących album ani nowych dostawców.
- MusicBrainz zwraca nagrania powiązane z wieloma wydaniami; zachowano wybór pierwszego wydania z poprawnie rozpoznanego nagrania. Nie gwarantuje to wydania rzeczywiście nadawanego przez stację. Dokładniejsza polityka albumów wymaga osobnej decyzji produktowej.
- Kodowanie historycznych ICY bez deklaracji jest niejednoznaczne. UTF-8/Latin-1 fallback nie rozwiązuje wszystkich kodowań. Brak metaint oznacza świadomie brak skanowania audio; niezgodne z ICY stacje wymagają odrębnych próbek diagnostycznych.
- Jedna instancja aplikacji/MainWindow ma jeden wspólny limit MusicBrainz. Gdyby w przyszłości powstało wiele niezależnych resolverów używających publicznego API równocześnie, należałoby współdzielić kolejkę na poziomie procesu. Aktualna aplikacja nie tworzy takiej konfiguracji.
- Nie testowano ręcznego odsłuchu, publicznych usług ani długich sesji. Nie zmieniano backendu audio/FFmpeg i nie powtarzano macierzy MP2. Pozostałe ostrzeżenia fontów/MiniPlayer są wcześniejsze i niezwiązane z tym zakresem.
