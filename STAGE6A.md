# Etap 6A — audyt i refaktoryzacja RadioList/Country

Data: 2026-11-19. Baza `b3de12b`, gałąź `refactor/codebase-audit`.
**Status: zakres 6A ukończony i zweryfikowany lokalnie na Windows.**
qmake i CMake budują aplikację; pełny QtTest z audio oraz CTest przechodzą.
Linux, zdalne CI i ręczna kontrola GUI pozostają nieweryfikowane, zgodnie z ograniczeniami opisanymi poniżej.

## Audyt przed zmianami

| Metody / grupa | Odpowiedzialność wejściowa | Zależności | Właściciel docelowy | Ryzyko |
| --- | --- | --- | --- | --- |
| RadioList: togglePlayback, playIndex, next/previous | Przekazanie intencji wybranej stacji | Widok/model, PlaybackController | RadioList jako adapter widoku, PlaybackController jako autorytet | Średnie: tożsamość i zmiana wierszy |
| Country: load, searchCountry, cancelStationSearch | Kraje, osobny model i asynchroniczne zapytania | RadioBrowserApi, widgets, RadioList progress | Country | Średnie: stare odpowiedzi i zamknięcie widoku |
| Oba showStationContextMenu | Powielone menu Play/Stop/Favorite/Details/homepage | PlaybackController, FavoriteManager przez RadioList, IconLoader | Jedna ścieżka menu w RadioList dla obu widoków | Wysokie: regresja P0 i generacja sesji |
| RadioList: requestPlaylist/search/display | Cache wyników katalogu i wybór playlisty | RadioBrowserApi, model, tree/table | RadioList | Średnie: powrót do listy i spóźniona odpowiedź |
| RadioList: onSessionChanged/details/metadata | Koordynacja usług i prezentacja sesji | PlaybackController, Reader/Recorder, RadioInfo, PlayerUIController | Istniejące komponenty; bez nowego kontrolera | Wysokie: kontrakt nagrywania podczas Pause |
| setFavorite/applyFavoritePlaylist | Intencja i aktualizacja widoku po zatwierdzonym zapisie | FavoriteManager | FavoriteManager zapisuje, RadioList aktualizuje UI | Wysokie: integralność danych |
| Model: data/setStations/icons | Dane tabeli, mapowanie tożsamości i ikony | RadioStation, IconLoader, QTableView | RadioStationsModel | Średnie: stare ikony i indeksy |
| MainWindow start/destruktor | Składanie i kolejność życia komponentów | Country, RadioList, Tray, resolver | MainWindow | Wysokie: kolejność niszczenia UI |
| PlayerUIController/Tray/Mini | Reprezentacja stanu z sygnałów kontrolera | PlaybackController, widgets | Istniejące komponenty | Średnie: rozjazd kontrolek |

Country jest potrzebny: obsługuje aktywny wybór kraju, osobny model, anulowanie zapytań i prezentację. Nie zawiera już własnego odtwarzacza. RadioList również nie ma lokalnej flagi Playing/Pause ani kopii aktywnego URL/stacji. `currentPlaylistIndex`, cache wyników, request IDs, tytuł ICY, kodek szczegółów i tekst statusu służą widokowi lub operacjom asynchronicznym; nie są autorytatywnym stanem odtwarzania. StreamRecorder pozostaje źródłem stanu zapisu. Nie usuwa się tych pól jako rzekomych flag PlaybackController.

Stan wejściowy testów z 5F: 84 zaliczenia, 0 błędów, 1 publiczny test pominięty; CTest 2/2. Wynik historyczny, nie nowy przebieg 6A.

## Podział odpowiedzialności po zmianach

- `PlaybackController` nadal wyłącznie określa stację, generację sesji i stan odtwarzania. Nie zmieniono jego kodu ani kontraktu Pause/Resume/Stop.
- `RadioList` obsługuje intencje użytkownika, wybór playlisty, cache wyników katalogu, wspólne menu oraz koordynację istniejących usług i prezentacji. Reader/Recorder pozostają osobnymi komponentami; ich stan nie jest kopiowany do flag GUI.
- `Country` wiąże kontrolki kraju z własnym modelem, uruchamia/anuluje asynchroniczne zapytania i przekazuje wybór stacji do tego samego kontrolera odtwarzania. Nie ma własnej sesji ani kopii stanu odtwarzacza.
- `RadioStationsModel` przechowuje dane tabeli i ikony widocznych stacji; udostępnia wspólne mapowanie indeksów przez modele pośredniczące. Porównanie tożsamości nadal korzysta z `FavoriteManager::sameStation`.
- `PlayerUIController`, Tray i MiniPlayer prezentują sygnały istniejącego kontrolera. `MainWindow` nadal składa komponenty i kontroluje ich normalną kolejność niszczenia.

Nie dodano klas ani nowych warstw. Pozostała koordynacja metadanych, szczegółów i nagrywania w RadioList jest świadomie zachowana, aby nie ponawiać refaktoryzacji z etapów 3B/5A.

## Wykonane zmiany i zależności

### RadioList

1. Cache wyników playlist i wskaźnik modelu przestały być publicznymi polami. Cache ma stały rozmiar odpowiadający rzeczywistym listom. Usunięto nieużywaną wartość `DISCOVERY` po sprawdzeniu wywołań. Tray nadal czyta dane przez `stations()`, a dostęp do modelu jest jawny przez `stationModel()`.
2. Usunięto zależność nagłówka od wygenerowanego `ui_mainwindow.h`; ten nagłówek dołączany jest w implementacji. Nie zmieniono generowania UI ani manifestu źródeł.
3. Dodano pamięć niezależnego wyboru użytkownika dla każdej listy. Zaznaczenie jest przywracane po powrocie lub resecie danych według tożsamości, bez uruchamiania odtwarzania. To celowy stan widoku, niezależny od aktywnej sesji.
4. Aktywacja, Play przy braku sesji, Next/Previous i kolorowanie korzystają z rzeczywistego modelu widoku. `stationAt()` mapuje indeks przez łańcuch `QAbstractProxyModel`, a `viewRowForStation()` znajduje pozycję aktywnej stacji w aktualnym widoku. Ukrycie/usunięcie stacji daje brak wyróżnionego wiersza, bez zatrzymania sesji.
5. `detachView()` anuluje własne żądania i ikonę szczegółów, odłącza aktualizacje GUI od kontrolera/usług i zamyka usługi strumienia. Najpierw unieważnia dostęp do UI. Samo usunięcie widgetów nie zmienia zewnętrznej sesji kontrolera; destruktor RadioList zachowuje dotychczasowe Stop. Obsługa nagrania pozostaje w istniejącym rejestratorze.
6. Wskaźniki katalogu i dialogu postępu są chronione przez `QPointer`; opóźnione wyświetlenie postępu oraz odpowiedzi list sprawdzają aktualny stan i identyfikator żądania.

### Country i wspólne menu

1. Zastąpiono przechowywany surowy `Ui::MainWindow*` wskaźnikami `QPointer` do potrzebnych kontrolek. Usunięto zbędne pole pixmapy; obraz nadal przechowuje QLabel.
2. `setData()` jest idempotentne dla tego samego powiązania. Ponowne wiązanie najpierw odłącza stare subskrypcje, unieważnia/anuluje żądania i usuwa stary model/delegat. Nie mnoży modeli i obsługi sygnałów.
3. Destruktor Country oraz usunięcie widoku lub RadioList kończą powiązanie. W obsłudze `destroyed` wskaźnik tabeli jest zerowany przed sprzątaniem, ponieważ interfejs pochodnego QWidget może już nie istnieć. To istotne także przy kolejności „widgety przed kontrolerami”.
4. Dwa menu stacji zastąpiono jedną istniejącą ścieżką w RadioList. Zachowano akcje i wygląd. Country przekazuje tylko tabelę i pozycję. Menu używa snapshotu stacji, trwałego indeksu oraz kontroli generacji przy Stop; nie zatrzyma nowej sesji wskutek akcji starego menu.
5. Menu jest własnością widoku i jest dodatkowo kontrolowane przez `QPointer`; zamknięcie widoku w trakcie `exec()` nie powoduje ponownego usuwania menu ani użycia usuniętego właściciela. Nie dodano blokującej pętli sieciowej. Dotychczasowe modalne menu/dialogi GUI pozostały.

### Modele i ikony

Dodano wspólne, niewielkie funkcje mapowania indeksów, bez zmiany przechowywania stacji. Model ikon rozpoznaje widoczne wiersze przez model pośredniczący i nadal przechowuje ikony według wierszy źródłowych oraz generacji danych. Sortowanie nie wyłącza pobierania ikon ani nie przypisuje odpowiedzi do innej stacji. Aktualizacja jest planowana także po odmalowaniu widoku, z użyciem istniejącego współdzielonego timera. Nie zmieniono IconLoader, jego cache ani limitów sieci/pamięci.

### Duplikacje stanu

Nie znaleziono lokalnych flag odtwarzania/pauzy ani własnego aktywnego URL w obu klasach. Nie usuwano poprawnych rozwiązań z 5A pod pretekstem duplikacji. Usunięto duplikację obsługi menu i niepotrzebny dostęp zapisu do cache. Zachowano identyfikatory żądań, cache list, stan ich załadowania, szczegóły/metadane i wybór użytkownika. Żadne z tych pól nie steruje autorytatywnym stanem odtwarzania.

## Testy regresyjne i pokrycie wymagań

Dodano 9 przypadków danych w 6 nowych funkcjach testowych:

| Test | Sprawdzane zachowanie |
| --- | --- |
| `testCountrySessionAcrossChanges` (Connecting / Playing) | Podwójna aktywacja nie dubluje sesji, szybka zmiana PL → GB → PL anuluje stare odpowiedzi, zmiana zakładki i zastąpienie modelu nie zmienia stacji/generacji; Stop emituje jedną zmianę sesji |
| `testListSelectionSurvivesNavigation` | Wybrany wiersz różny od odtwarzanej stacji; powrót Favorites → Top → Favorites, zmiana kolejności i wyszukiwanie zachowują wybór według tożsamości i nie zmieniają sesji |
| `testStationProxyIdentity` (RadioList / Country) | Rzeczywiste sortowanie/filtrowanie przez QSortFilterProxyModel, poprawna stacja i wyróżnienie, brak nowej sesji po zmianie indeksów, puste/obce indeksy są ignorowane |
| `testViewDestructionCancelsRequests` (Country / widgety przed kontrolerami) | Anulowanie częściowych żądań, usunięcie modelu, brak callbacków do zniszczonych kontrolek, brak restartu sesji i zamknięcie połączeń HTTP |
| `testCountryRebind` | Anulowanie starego żądania, idempotentne setData/load, jeden model i brak aktualizacji starego UI |
| `testStationMenuClosesWithView` | Usunięcie okna w czasie otwartego menu, bez awarii i bez przypadkowego Play |

Rozszerzono `testTableImagesFollowStationIdentity` o drugi wariant `sorted-proxy`: reset danych z oczekującą starą odpowiedzią, sortowanie podczas dwóch aktywnych pobrań, odwrotna kolejność odpowiedzi, poprawne kolory ikon w wierszach widoku i po jednym GET na URL. Wariant bez proxy zachowano. Łącznie zestaw ma 10 dodatkowych przypadków danych względem 5F.

`testPlaybackViewsAndTray` zasila teraz listę przez kontrolowany lokalny HTTP zamiast bezpośredniej modyfikacji publicznego cache. Zachowano jego asercje MainWindow UI/Tray/MiniPlayer. Pozostałe odwołania do modelu w testach zmieniono na jawny accessor.

| Kategoria z zadania | Pokrycie |
| --- | --- |
| Wybór w RadioList | `testStationProxyIdentity`, `testListSelectionSurvivesNavigation`, istniejący `testPlaybackModelIdentity` |
| Kraj podczas Playing i Connecting | Dwa warianty `testCountrySessionAcrossChanges` |
| Sortowanie/zmiana indeksów | Oba warianty `testStationProxyIdentity`, rozszerzony test ikon, istniejący `testPlaybackModelIdentity` |
| Usunięcie Favorites w trakcie Play | Zachowany `testPlaybackModelIdentity`: stan, URL, generacja, liczba startów i brak wyróżnienia |
| Stara odpowiedź Radio Browser | Nowy test krajów/rebind i zachowane `testCatalogSearchReplacementAndCancellation`, `testCatalogListsDoNotOverwriteVisibleSelection` |
| Zamknięcie widoku podczas żądania | Oba warianty testu niszczenia, test menu i zachowany test zamykania widoku ikon |
| Sygnały Play/Pause/Stop | Zachowane `testRadioListPause`, `testPlaybackSwitchAndStop`, `testPlaybackStopDuringBuffering`, regresja P0 oraz nowy test krajów |
| Synchronizacja MainWindow/Tray/Mini | Zachowany i dostosowany `testPlaybackViewsAndTray` |
| Zmiana listy bez nowej sesji | Nowy test nawigacji oraz zachowane testy tożsamości/Tray |
| Powrót do poprzedniej listy | `testListSelectionSurvivesNavigation`, powrót zakładki w teście krajów |
| Puste modele/nieprawidłowe indeksy | Dwa warianty `testStationProxyIdentity` i zachowany `testPlaybackModelIdentity` |

Wszystkie nowe testy używają lokalnego serwera i kontrolowanych danych. Zestaw uruchamia się we własnym `QTemporaryDir`, a testy nagrywania/Favorites mają katalogi tymczasowe. Nie korzystano z danych użytkownika ani publicznych stacji. Zachowano test P0 `testStopCountryStationFromContextMenu`, testy rejestratora oraz trzy opcjonalne testy rzeczywistego backendu multimediów i sześć przypadków osobnego audio_diagnostics.

## Weryfikacja

Środowisko: Windows x64, Qt/Qt Multimedia **6.12.0** z `C:\Qt\6.12.0\mingw_64`, **GCC 15.1.0** z `C:\Qt\Tools\mingw1510_64\bin`, C++17, CMake 4.1.2/Ninja. Log backendu testów potwierdza FFmpeg **9.0.1**. PATH i QT_PLUGIN_PATH wskazują ten zestaw; nie zmieniono konfiguracji produkcyjnej ani instalacji Qt.

| Weryfikacja | Wynik |
| --- | --- |
| qmake, osobny świeży katalog, Release, aplikacja i testy | PASS |
| Pełny QtTest, `TUNESCAPE_TEST_MEDIA=0`, końcowy zestaw | 91 PASS / 0 FAIL / 4 SKIP, 16,505 s |
| Pełny QtTest, `TUNESCAPE_TEST_MEDIA=1`, końcowy zestaw | 94 PASS / 0 FAIL / 1 SKIP, 39,805 s |
| Świeży CMake/Ninja, aplikacja, testy, audio_diagnostics | PASS; końcowe rozszerzenie testu także skompilowane |
| Pełny CTest z audio, końcowy zestaw | 2/2 PASS, 77,72 s; QtTest exit 0 (45,70 s), audio_diagnostics 8/0/0 |
| `git diff --check` | PASS |
| Linux | Niewykonany: `wsl --list --verbose` zgłasza brak zainstalowanych dystrybucji (`WSL_E_DEFAULT_DISTRO_NOT_FOUND`) |
| GitHub Actions | Nie uruchamiano; brak deklaracji wyniku zdalnego workflow |

Przed ostatnim rozszerzeniem testu ikon pełny qmake QtTest dał 90/0/4 (domyślny) i 93/0/1 (media), a CTest z audio 2/2 w 75,49 s. Są to rzeczywiste przebiegi 6A; nie zastępują weryfikacji końcowego zestawu.

Porównanie z historycznym 5F: 84 → 94 zaliczenia w trybie media, nadal 0 błędów i 1 pominięcie. Przyrost o 10 odpowiada nowym przypadkom, a nie usunięciu dotychczasowych testów. Końcowy zestaw przeszedł zarówno w qmake, jak i ponownie w CTest z audio.

Licznik 94/0/1 pochodzi z jawnego logu QtTest builda qmake. CTest potwierdził kod 0 drugiego builda; jego LastTest.log nie zawiera tekstowego podsumowania TuneScapeTests (zawiera podsumowanie audio_diagnostics). Nie utożsamia się braku stdout w tym logu z pominięciem testów.

Prace prowadzono etapami. Pierwszy build/test po rozdzieleniu nagłówka i wspólnym menu dał 81/0/4. Pierwsze nowe testy ujawniły brak odświeżenia wyróżnienia po filtracji oraz błąd kolejności sprzątania QWidget; poprawiono oba. Test zamknięcia doprecyzowano, aby czekał na rzeczywiste żądania przed sprawdzaniem zwolnienia połączeń. Po poprawkach wszystkie nowe przypadki przechodziły w osobnym przebiegu 10/0/0 (wraz z init/cleanup; przed dodaniem testu menu). Zachowano logi, w tym ślad GDB z pośredniej awarii. Nie przedstawia się ich jako wyników finalnego kodu.

Jedyny SKIP w trybie media dotyczy publicznych stacji wymagających `TUNESCAPE_LIVE_STREAM_A/B`. W trybie domyślnym dodatkowo pomijane są trzy testy rzeczywistego backendu. Media sprawdzają lokalne HTTP, przełączanie, Pause/Resume/Stop, nagrywanie, błędy i zwalnianie połączeń. Nie wykonywano ręcznego odsłuchu, fizycznego uśpienia Windows ani ponownej macierzy MP2 z 5E.

Wspólny manifest `TuneScapeCore/sources.list` zawiera wszystkie sześć zmienionych plików produkcyjnych i jest używany przez `common.pri` oraz główny CMakeLists. `uic` i `moc` wykonano w obu buildach. Nie wymagało to zmian konfiguracji budowania.

Buildy i dowody są poza repozytorium, w:
`C:\Users\gkedzior\AppData\Local\Temp\TuneScape-stage6a-20261119`.
Najważniejsze pliki: `qmake/final-extended-build.log`, `full-final-default.txt`, `full-final-media.txt`, `cmake-configure.log`, `cmake-final-build.log`, `ctest-final-media.txt`, `cmake/Testing/Temporary/LastTest.log`. Wyniki etapów 5C–5F pozostawiono bez zmian.

## Zmienione pliki

| Plik | Zakres |
| --- | --- |
| `TuneScapeCore/include/radiolist.h` | Ukrycie cache/modelu, stan wyboru list, deklaracja wspólnego menu, ograniczenie zależności UI |
| `TuneScapeCore/src/radiolist.cpp` | Menu, mapowanie stacji, wybór, strażnicy czasu życia i anulowanie operacji |
| `TuneScapeCore/include/Country.h` | Typowane QPointer, jawne sprzątanie i ewidencja połączeń |
| `TuneScapeCore/src/Country.cpp` | Wiązanie widoku, anulowanie żądań, wspólne menu, tożsamość stacji |
| `TuneScapeCore/include/model/RadioStationsModel.h` | Wspólne funkcje mapowania indeksów i tożsamości |
| `TuneScapeCore/src/model/RadioStationsModel.cpp` | Implementacja mapowania i ikony widocznych wierszy przez proxy |
| `TuneScapeTests/tst_tunescapetests.cpp` | Nowe i rozszerzone regresje, testy korzystające z jawnego dostępu do modelu |
| `STAGE6A.md` | Audyt, zakres, dowody, ograniczenia i rekomendacje |

Nie zmieniono PlaybackController, RadioAudioManager, StreamRecorder/Reader, RadioBrowserApi, MusicBrainz, Favorites/M3U, IconLoader, motywów, UI ani konfiguracji Qt/deploymentu. Nie wykonano commit, push, merge ani zmiany gałęzi. Początkowe drzewo było czyste; nie nadpisano niezwiązanych zmian.

## Ryzyka, pozostałe zadania i rekomendacja

- RadioList nadal korzysta z `Ui::MainWindow*` w implementacji; zabezpieczenie opiera się na czasie życia głównego `centralwidget`. Country jest od tego wskaźnika odłączony. Dynamiczne usuwanie pojedynczych kontrolek bez usuwania głównego widgetu nie jest kontraktem aktualnej aplikacji ani nową funkcją.
- Obiekty przekazane referencjami/accessorami (`playback()`, `catalog()`) powinny żyć podczas używania ich przez klientów. Standardowe składanie MainWindow i testy niszczenia zachowują ten kontrakt; nie przebudowywano całej własności usług.
- Mapowanie aktywnej stacji do wiersza jest liniowe względem widocznej listy. Typowe listy są krótkie; nie dodawano dodatkowego indeksu/cache tożsamości. Planowanie ikon przy Paint jest scalane timerem, ale przy bardzo dużych modelach warto osobno zmierzyć obciążenie przewijania.
- Testy GUI są wykonywane offscreen na rzeczywistych widgetach, a nie przez ręczne klikanie produkcyjnego MainWindow. Przed wydaniem pozostają kontrola wizualna motywów/DPI, interakcje z systemowym Tray oraz test natywnego Linux i wynik GitHub Actions.
- Nadal pojawiają się wcześniejsze ostrzeżenia o stylesheet MiniPlayer i katalogu fontów Qt. Nie zmieniano tych niezwiązanych elementów. Oczekiwane komunikaty FFmpeg z testów HTTP 404/5xx/anulowania nie są błędami testów.
- Wyniki nie są dowodem pełnej stabilności natywnego backendu dla dowolnych kodeków ani długich sesji. Nie deklaruje się naprawy MP2 mono w ramach 6A.

Rekomendacja: utrzymać obecny podział i najpierw potwierdzić Linux/CI oraz ręczne scenariusze GUI. Ewentualne dalsze ograniczenie zależności RadioList od UI i koordynacji metadanych powinno być osobnym małym etapem z wykazanym problemem, bez kolejnej przebudowy kontrolera odtwarzania. Nie jest wymagane do zachowania funkcjonalności w zakresie 6A.
