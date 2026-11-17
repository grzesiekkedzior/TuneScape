# TuneScape — raport etapu 5B

Weryfikacja na Windows 11, Qt 6.7.0 i MinGW 11.2. Gałąź `refactor/codebase-audit`, baza `7112fef`. Raport z 17 listopada 2026.

## Wyniki

| Próba | Wynik |
| --- | --- |
| Pełny QtTest z włączonymi lokalnymi testami multimediów, qmake | Dwa przebiegi po 81 zaliczeń, 0 niepowodzeń i 1 pominięciu publicznego testu |
| CMake i CTest, tryb domyślny | 1/1, lokalne testy multimediów i publiczny test jawnie pomijane |
| CMake i CTest, TUNESCAPE_TEST_MEDIA=1 | 1/1, 81 zaliczeń i 1 pominięcie publicznego testu |
| A → B → A, szybka zmiana podczas Connecting i sześć Pause/Resume na lokalnym HTTP | Zaliczone na rzeczywistym backendzie Qt, z postępem pozycji odtwarzacza |
| Powtarzanie dwóch lokalnych testów cyklu życia na kodzie produkcyjnym 5A | Sześć procesów, każdy z sześcioma Pause/Resume, wszystkie zaliczone dla stereo MP3 |
| Nagrywanie lokalnego HTTP podczas Play i Pause | Zaliczone, plik ma dokładnie odebrane bajty |
| Stop i zmiana stacji podczas nagrywania | Zaliczone, zapisane pliki ponownie odtwarzane przez rzeczywisty backend |
| Nieprawidłowy URL, 404, 503 i serwer bez odpowiedzi | Zaliczone, kończą się Error, zasoby są zwalniane |
| Przedwczesne zamknięcie odpowiedzi HTTP podczas Playing | Qt/FFmpeg przeszedł do Stopped. Ponowne Play zadziałało |
| Publiczne Radio Paradise HTTP i SomaFM HTTPS | Dekodowanie A → B → A i dwa Pause/Resume zaliczone |
| Publiczne Radio Paradise, zapis podczas dwóch Pause/Resume i finalizacja po zmianie stacji | Zaliczone w rozszerzonym teście |
| Publiczne SomaFM, odtwarzanie | Zaliczone |
| Publiczne SomaFM, nagrywanie przez QNetworkAccessManager | Niezaliczone, HTTP 403 Forbidden |
| Linux | WSL Ubuntu 24.04 bez qmake6, qt6-base-dev, qt6-multimedia-dev i qt6-svg-dev. Brak builda |
| git diff --check | Zaliczone |

Publiczny test ma wynik końcowy niepowodzenia z powodu HTTP 403 przy nagrywaniu drugiej stacji. Nie jest zaliczany przez lokalny CTest. Publiczne próby wykonano osobno, poza ograniczeniem sieciowym sandboxa.

Adresy użyte w próbie publicznej to `http://stream.radioparadise.com/mp3-320` i `https://ice6.somafm.com/gsclassic-128-mp3`. Ich dostępność oraz odpowiedzi mogą zmieniać się niezależnie od aplikacji.

## Zasoby i synchronizacja

Testy sprawdzają postęp pozycji rzeczywistego QMediaPlayer, usunięcie poprzednich instancji przez QPointer, jedną aktywną instancję po przełączeniu i brak instancji po Stop. QAudioOutput pozostaje pojedynczym obiektem należącym do RadioAudioManager, zgodnie z etapem 5A.

Lokalny serwer sprawdza liczbę otwartych połączeń. Sam tor audio używa jednego. Integracja z RadioList używa dwóch podczas Play, trzech po rozpoczęciu nagrania, dwóch po Pause i zera po Stop. Sześć cykli Pause/Resume nie mnoży połączeń. Testy sprawdzają też dokładną liczbę GET-ów.

Dotychczasowe regresje nadal sprawdzają MainWindow, Country, MiniPlayer i Tray, zmianę list i kolejności wierszy, usunięcie Favorites, nieaktualny błąd, nieaktualne metadane, zamykanie oraz logiczne suspend/resume. To testy kontrolowanych sygnałów. Fizyczne uśpienie oraz ręczny odsłuch pozostają do wykonania.

Nie przeprowadzono profilowania sterty ani długotrwałego pomiaru pamięci. Zwalnianie obiektów i połączeń w sprawdzonych cyklach potwierdzono, ale wynik nie wyklucza wszystkich wycieków wewnątrz Qt lub kodeków.

## Wykryta awaria backendu

Przy testowaniu mono MPEG Layer II i szybkich zmian sesji wystąpił SIGSEGV. GDB wskazał wątek Qt/FFmpeg i stos przechodzący przez swr_convert() w swresample-4.dll oraz ffmpegmediaplugin.dll. Awaria była nieregularna.

Próbna zmiana na osobny QAudioOutput dla każdej próby również uległa awarii. Wycofano ją po porównaniu. Kod produkcyjny zachowuje publiczne API i mechanizm etapu 5A. Nie ustalono dostatecznie przyczyny, by uzasadnić poprawkę architektury. W testach pozostawiono opcjonalny wariant mono do dalszej diagnostyki. Cztery końcowe powtórzenia samego przełączania mono przeszły, co potwierdza nieregularność problemu.

Początkowy fixture WAV nie nadawał się do symulowania ciągłego radia. Backend traktował go jako zakończony materiał. Lokalny podstawowy test używa teraz generowanego stereo MP3 z kontrolowanym HTTP, bez publicznych usług i binarnych fixture'ów w repozytorium.

## Uruchomienie testów

Testy domyślne są niezależne od publicznego internetu. Lokalne testy rzeczywistego backendu wymagają biblioteki Qt Multimedia i dostępnego wyjścia audio. Są wyciszone.

Przykład PowerShell uruchamiany w katalogu builda CMake po skonfigurowaniu PATH do Qt i MinGW.

```powershell
ctest --output-on-failure --no-tests=error

$env:TUNESCAPE_TEST_MEDIA = '1'
ctest --output-on-failure --no-tests=error
```

Publiczne próby należy wykonywać osobno w środowisku z dostępem do sieci. W qmake należy podać ścieżkę do `TuneScapeTests/release/TuneScapeTests.exe`.

```powershell
$env:TUNESCAPE_LIVE_STREAM_A = 'http://stream.radioparadise.com/mp3-320'
$env:TUNESCAPE_LIVE_STREAM_B = 'https://ice6.somafm.com/gsclassic-128-mp3'
.\TuneScapeTests.exe testLiveRadioStreams -o live-radio.txt,txt
```

Wariant diagnostyczny mono może zakończyć proces awarią i powinien działać w osobnym procesie pod debuggerem.

```powershell
$env:TUNESCAPE_TEST_MEDIA = '1'
$env:TUNESCAPE_TEST_MONO = '1'
.\TuneScapeTests.exe testMediaHttpSwitchAndRelease -o mono.txt,txt
```

## Zmienione pliki i dalsze kroki

Zmiany obejmują `TuneScapeTests/tst_tunescapetests.cpp` i niniejszy raport. Testy używają QTemporaryDir, generowanego audio i lokalnego HTTP. Publiczne nagrania trafiają do tymczasowego katalogu testu. Dane użytkownika i istniejące nagrania nie były używane.

Etap 5C może rozpocząć diagnostykę resamplera mono oraz różnicy pomiędzy dostępem FFmpeg i QNetworkAccessManager do SomaFM. Pełna gotowość wydaniowa wymaga jeszcze ręcznego odsłuchu, fizycznego suspend/resume Windows, prób utraty rzeczywistej sieci oraz testów Linux KDE/GNOME pod Waylandem i X11. Warto porównać reproduktor z nowszym Qt/FFmpeg i wykonać dłuższy pomiar pamięci.

W trakcie pracy nie wykonano commita, push ani merge.
