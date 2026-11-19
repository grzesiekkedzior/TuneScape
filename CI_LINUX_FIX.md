# CI-Linux — poprawka zgodności z Qt 6.4 i weryfikacja

Data: 2026-11-19. Gałąź `refactor/codebase-audit`, HEAD `87db5ad03877a3dca92fa632b55eeb04c74b3253`. Początkowe drzewo robocze było czyste.

**Status: minimalna poprawka potwierdzonej niezgodności API Qt 6.4 przygotowana i lokalnie zweryfikowana. Końcowa regresja Windows PASS; ostateczne potwierdzenie Linux oczekuje na GitHub Actions.** Nie deklaruje się zaliczenia builda Linux. Wklejony fragment logu nie zawiera pełnego pierwszego komunikatu kompilatora, ale wskazuje ResolverService.o i przeciążenia QDebug; niezgodność wskazanego kodu odtworzono niezależnie kontrolą składni z oficjalnym nagłówkiem QDebug 6.4.2.

## Przeanalizowane przebiegi

Publiczne API GitHub potwierdziło:

| Przebieg | Commit | Linux | Windows |
| --- | --- | --- | --- |
| [#11, 44541402213](https://github.com/grzesiekkedzior/TuneScape/actions/runs/44541402213) | `b3de12b5c8c5b1abe6647a9f5a4c00495f43519c` | Job `135620394911`: instalacja PASS, Build with qmake FAIL, QtTest SKIP | Job `135620394845`: instalacja Qt FAIL; kompilacja/testy nie wystartowały |
| [#12, 44624910039](https://github.com/grzesiekkedzior/TuneScape/actions/runs/44624910039) — najnowszy odczytany | `87db5ad03877a3dca92fa632b55eeb04c74b3253` | Job `135916019540`: instalacja PASS, Build with qmake FAIL, QtTest SKIP | Job `135916020143`: instalacja Qt FAIL; kompilacja/testy nie wystartowały |

Nie ograniczono diagnozy do historycznego #11: #12 zawiera już etap 6A i nadal kończy się błędem kompilacji Linux.

## Błąd, dowody i granice diagnozy

Początkowo adnotacje Linux obu przebiegów zawierały jedynie `Process completed with exit code 2.`. Następnie użytkownik dostarczył fragment kroku Build with qmake #12: `C:\Users\gkedzior\.codex\attachments\01cab135-f4bb-46db-a335-e169fed0a871\Wklejony tekst.txt`. Potwierdza on **Qt 6.4.2, GCC 13.3.0**, generowanie UI/moc oraz zakończenie błędem przy `ResolverService.o`, poprzedzone listą kandydatów `QDebug::operator<<`.

Fragment pomija środkową część wyjścia, w tym pierwszy komunikat `error:`. Nie przypisuje się mu dosłownego komunikatu, którego nie zawiera, ani nie stwierdza, że wyklucza inne równoległe błędy kompilacji.

W `ResolverService::checkReady()` znaleziono:

```cpp
qDebug() << "Cover URL:" << m_pending.track.coverUrl.toStdString();
```

`coverUrl` jest QString, a konwersja daje std::string. Obsługa `std::basic_string` w QDebug została dodana w **Qt 6.5** ([dokumentacja Qt](https://doc.qt.io/qt-6/qdebug.html)). Oficjalny [nagłówek Qt 6.4.2](https://github.com/qt/qtbase/blob/v6.4.2/src/corelib/io/qdebug.h) nie zawiera tego przeciążenia; lokalny Qt 6.12 je zawiera. To niezgodność używanego API, a nie dowód problemu FFmpeg, MinGW/GCC lub zmiany architektury z 6A.

**Niezależnie odtworzony pierwszy błąd minimalnej próby składniowej:**

```text
error: ambiguous overload for 'operator<<' (operand types are 'QDebug' and 'std::string' ...)
```

Ta próba używa oficjalnego qdebug.h 6.4.2 i pozostałych nagłówków lokalnego Qt 6.12 z GCC 15.1, wyłącznie `-std=c++17 -fsyntax-only`. Wariant pierwotny kończy się kodem 1, wariant z bezpośrednim QString kodem 0. Nie linkuje ani nie uruchamia mieszanych bibliotek. **Nie jest to pełny build Qt 6.4 ani build/test Linux.** Potwierdza brak odpowiedniego przeciążenia i skuteczność zmiany dla wskazanego wyrażenia; całość projektu nadal wymaga CI.

Próby pobrania pełnych logów przez `GET /repos/grzesiekkedzior/TuneScape/actions/jobs/{id}/logs` dla obu jobów Linux oraz Windows #12 zwróciły HTTP 403. Publiczny odczyt metadanych i adnotacji działał. Próba odczytu strony joba narzędziem web nie dostarczyła logu.

Automatyczna kontrola uprawnień odrzuciła polecenie, które miało odczytać zapisane poświadczenie przez `git credential fill` i użyć go wyłącznie do pobrania logów. Uzasadnienie: zgoda na analizę logów nie została uznana za zgodę na odczyt/użycie lokalnego sekretu. Polecenie nie zostało uruchomione; poświadczenie nie zostało odczytane. Nie obchodzono tej blokady.

Do dalszej pracy wykorzystano dostarczony tekst oraz oficjalny nagłówek i dokumentację Qt; nie potrzebowano dostępu do poświadczeń. Kod 2 make jest skutkiem nieudanej kompilacji jednostki, a ostrzeżenie unused parameter w RadioExplorer jest osobną, niekrytyczną uwagą.

## Środowisko i audyt

Obecny workflow Linux deklaruje `ubuntu-24.04` i instaluje dystrybucyjne Qt 6/GCC przez apt. Wersje pakietów nie są przypięte. Log #12 potwierdza Qt 6.4.2 w `/usr/lib/x86_64-linux-gnu`, qmake 3.1 i GCC 13.3.0; dokładnych wersji z #11 nie odczytano. Windows lokalnie używa Qt/Multimedia 6.12.0 i GCC 15.1.0. Zachowano minimalne Qt 6.4 w CMake.

Dotychczasowe komendy: `qmake6 -v`, `g++ -dumpfullversion`, `mkdir build`, `cd build`, `qmake6 ../TuneScape.pro CONFIG+=release`, `make -j4`. QtTest miał działać jako `./build/TuneScapeTests/TuneScapeTests`; globalne `QT_QPA_PLATFORM=offscreen` już było ustawione.

Kontrola statyczna wykazała:

- Wspólny `TuneScapeCore/sources.list` jest używany przez qmake i CMake. Wszystkie wpisy istnieją w plikach śledzonych przez Git, z identyczną wielkością liter.
- Wyszukane lokalne include odpowiadają śledzonym ścieżkom z zachowaniem wielkości liter. Wyjątki automatycznej kontroli to `qjsondocument.h` w dwóch klientach MusicBrainz — to nagłówek Qt, nie brakujący plik projektu; nie modyfikowano tych klientów. Wygenerowane `ui_*.h` i `.moc` są obsługiwane przez build.
- W widocznych fragmentach Win32 jest chronione przez `Q_OS_WIN`, a linkowanie `psapi/dbghelp` w diagnostyce ograniczono do Windows. Nie stanowi to dowodu kompilowalności całego projektu na Linux.
- C++17 jest deklarowany w common.pri i CMake. CMake wymaga Qt co najmniej 6.4. Nie zmieniono minimalnej wersji bez dowodu niezgodności.
- Sprawdzono występowanie `iconSizeChanged` w oficjalnym nagłówku Qt 6.4.2 i lokalnym Qt 6.12 — sygnał jest obecny w obu. Nie uznano tej hipotezy za przyczynę awarii.

WSL zgłosił `WSL_E_DEFAULT_DISTRO_NOT_FOUND`, brak zainstalowanych dystrybucji. Nie znaleziono poleceń Docker, Podman ani Multipass. Dostępny bash.exe jest komponentem Windows/WSL, a MSYS2 jest środowiskiem Windows; nie uznano ich za natywny Linux z Qt. Nie instalowano dystrybucji ani dużego środowiska, nie wykonywano połączeń do nieuzgodnionych hostów SSH.

## Zmienione pliki i uzasadnienie

1. `.github/workflows/qmake.yml` — wyłącznie job Linux:
   - jawne `TUNESCAPE_TEST_MEDIA=0` utrzymuje deterministyczny kontrakt CI bez wymagania urządzenia audio;
   - jawna instalacja CMake/Ninja i pluginów QPA do offscreen;
   - zapis Qt, GCC, systemu oraz wyjścia qmake/make do plików;
   - `shell: bash` daje standardowe Actions `-e -o pipefail`, więc `tee` nie zamaskuje niepowodzenia kompilatora;
   - zachowany pełny QtTest, z wyjściem tekstowym oraz JUnit w formacie `junitxml`, sprawdzonym w parserze argumentów Qt 6.4.2;
   - osobny build CMake/Ninja i CTest, `--no-tests=error` oraz JUnit;
   - upload logów `if: always()`, także przy nieudanej kompilacji.
2. `TuneScapeCore/src/music_data/services/ResolverService.cpp` — usunięto wyłącznie zbędną konwersję `.toStdString()` przy logowaniu Cover URL i dodano jawny include QDebug. Bezpośredni QString działa w Qt 6.4 i 6.12. Nie zmieniono przepływu MusicBrainz, danych Track, sygnałów, zapytań ani przetwarzania wyników. To bezpośrednio uzasadniona poprawka kompilacji w komponencie wskazanym przez log.
3. `CI_LINUX_FIX.md` — stan dowodów, wyniki, ograniczenia i konkretna dalsza procedura.

Job Windows pozostał identyczny. Nie dodano `continue-on-error`, pomijania testów ani pozornej kontroli w miejsce kompilacji. Brak zmian PlaybackController, RadioList/Country, rejestratora, klientów MusicBrainz, konfiguracji Qt 6.12 i deploymentu. Nie podnoszono minimalnej wersji Qt dla pojedynczej instrukcji diagnostycznej.

Zmiany workflow zapewniają kompletne dowody i dwa rzeczywiste buildy. Poprawka źródłowa usuwa odtworzoną niezgodność QDebug; sukces całego Linux nie jest jeszcze potwierdzony. Nie dodawano testu odtwarzającego samą instrukcję logowania w repozytorium: problem weryfikuje kompilator, a istniejące regresje zachowano.

## Wyniki lokalne

Dowody zapisano poza repozytorium w `C:\Users\gkedzior\AppData\Local\Temp\TuneScape-ci-linux-20261119`.

| Kontrola | Rzeczywisty wynik |
| --- | --- |
| Kontrola składni z QDebug 6.4.2 | Przed: exit 1, ambiguous operator<<; po: exit 0. Ograniczenia próby opisane powyżej |
| Windows qmake, Qt 6.12.0, GCC 15.1.0, po poprawce | PASS, ponowna generacja, kompilacja ResolverService i linkowanie aplikacji/testów Release w istniejącym izolowanym katalogu 6A |
| Pełny Windows QtTest po poprawce, `TUNESCAPE_TEST_MEDIA=1` | **94 PASS, 0 FAIL, 1 SKIP**, 46,231 s; exit 0 |
| Windows CMake/Ninja po poprawce | PASS, kompilacja zmienionego źródła dla obu targetów i ponowne linkowanie |
| Windows CTest po poprawce, z audio | **2/2 PASS**, 77,24 s (QtTest 45,76 s; audio_diagnostics 31,30 s) |
| Kontrola tekstowego/JUnit loggera polecenia QtTest | PASS: test P0 wraz z init/cleanup 3/0/0, 247 ms, exit 0; JUnit poprawnie odczytany parserem XML |
| Linux qmake / QtTest | **Niewykonane** — brak kompletnego lokalnego środowiska |
| Linux CMake / CTest | **Niewykonane** — jw. |
| actionlint 1.7.12 | PASS, exit 0; shellcheck/pyflakes wyłączone, bo nie są dostępne |
| `git diff --check` | PASS |
| Nowy przebieg GitHub Actions z tymi zmianami | **Nie uruchomiono** |

Przed zmianą ResolverService także uruchomiono regresję: qmake PASS, QtTest 94/0/1 w 45,540 s, CMake aktualny, CTest 2/2 w 76,22 s. Powyższa tabela opisuje wyniki końcowe po poprawce. Nie przedstawia się istniejących katalogów build jako świeżych buildów tego zadania. Po zmianie ponownie skompilowano zmienione jednostki i linkowano aplikację/testy. Pełnej macierzy MP2 nie powtarzano; CTest wykonał istniejącą krótką diagnostykę audio.

Główne dowody końcowe: `postfix-qmake-configure.log`, `postfix-qmake-build.log`, `postfix-qttest-media.txt`, `postfix-cmake-build.log`, `postfix-ctest.txt`, `actionlint-final.txt` (pusty przy poprawnym przebiegu), `qmake-junit.xml`, `qmake-junit-stdout.txt`, `qdebug-api-check.cpp`, `qdebug-api-before.txt`, `qdebug-api-after.txt`, `qdebug-6.4.2.h`, `qtestcase-6.4.2.cpp`. Wcześniejsze dowody `windows-*.log/txt` i `annotations-*.json` zachowano. Szczegóły CTest pozostają także w buildzie 6A `cmake/Testing/Temporary/LastTest.log`. Zachowano wszystkie wcześniejsze wyniki etapów 5D–6A.

## Ostrzeżenia i osobny problem Windows CI

Lokalne testy nadal zgłaszają wcześniejsze ostrzeżenia stylesheet MiniPlayer/fontów; jedno pominięcie dotyczy opt-in publicznych stacji. Nie zmieniano tych elementów. Git informuje o przyszłej konwersji LF do CRLF; `diff --check` nie zgłasza błędów białych znaków.

Windows CI #12 zatrzymał się podczas instalacji, a nie na kompilacji C++. Publiczna adnotacja pokazuje zakończenie procesu Python 3.15.2 kodem 1, bez pierwotnego komunikatu instalatora. Nie uznano samego tego faktu za dowód niezgodności Pythona/aqt. Potrzebny jest pełny log `Install Qt 6.12 and MinGW 15.1`. Adnotacje obejmują też ostrzeżenie o wersji Node upload-artifact i brak plików do uploadu po nieudanej instalacji. Nie zmieniono joba Windows bez diagnozy jego osobnego problemu.

## Co konkretnie pozostaje

1. Potwierdzić cały Linux na przygotowanej poprawce: qmake + pełny domyślny QtTest oraz osobny CMake + CTest, lokalnie na uzgodnionym istniejącym środowisku albo w nowym Actions. Zaktualizowany job zapisze logi także w razie błędu.
2. Jeżeli build nadal nie przejdzie, odczytać pierwszy `error:` z nowego artifactu `linux-build-and-test-logs`, bez wnioskowania z wtórnego kodu make. Nie zakłada się, że fragment #12 pokazywał wszystkie błędy.
3. Opcjonalnie uzupełnić archiwum o pełne logi #11/#12 dla dokładnego porównania pierwszych błędów; nie wymaga to dostępu do sekretów, jeżeli użytkownik pobierze pliki przez GitHub.
4. Zdiagnozować oddzielny błąd instalacji Windows CI na podstawie pełnego logu instalatora; lokalny sukces Windows go nie rozstrzyga.
5. Potwierdzić oba joby na nowym przebiegu. Workflow ma wyłącznie push na `refactor/codebase-audit` i pull_request, brak workflow_dispatch. Ponowne uruchomienie starego przebiegu nie sprawdzi lokalnych niecommitowanych zmian. Nie wykonano rerun, commit, push, merge ani zmiany gałęzi.

**GitHub Actions nie potwierdził naprawy.** Najnowszy odczytany wynik to nadal niepowodzenie #12. Poprawka API ma dowód lokalny, ale raport wymaga uzupełnienia o rzeczywisty wynik całego Linux i instalacji Windows CI. Samo przejście actionlint nie stanowi wyniku workflow.
