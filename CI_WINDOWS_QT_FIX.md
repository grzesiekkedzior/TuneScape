# Windows CI #13 — instalacja Qt 6.12 i MinGW 15.1

Gałąź: `refactor/codebase-audit`. Baza: `7af2aba21c97a95ece1a4e353c1025966e6f8e12`.

**Status: poprawka instalatora przygotowana, kontrola zależności i dry-run zakończone sukcesem. Pełny Windows CI pozostaje niepotwierdzony.** Nie zmieniono produkcyjnego C++, Qt 6.12.0, GCC 15.1.0 ani deploymentu.

## Przebieg i dostępne dowody

Przeanalizowano [przebieg #13, 44628481153](https://github.com/grzesiekkedzior/TuneScape/actions/runs/44628481153):

- Windows job `135928502614`: FAIL podczas instalacji Qt, przed kompilacją i testami.
- Linux job `135928502169`: SUCCESS, w tym qmake, QtTest i CMake/CTest. To rzeczywisty wynik GitHub Actions dla commita z poprawką Qt 6.4; nie jest lokalnym buildem Linux.
- Akcja install-qt-action z joba: `b344308e4483fc892e0d27b3cd1b173ebdd95f06` (`v4`). Jej action.yml wybiera Python `>=3.14.0`; według tekstu użytkownika oraz adnotacji wybrano Python 3.15.2, aqtinstall 3.3.0.

Próba pobrania pełnego logu Windows przez publiczne API `/actions/jobs/135928502614/logs` zwróciła HTTP 403. Odczytano publiczne metadane i adnotacje oraz tekst użytkownika. Ten tekst zawiera działające `aqt version`, nazwę grupy Install Qt i końcowy wyjątek JavaScript, ale nie wewnętrzne polecenie/ERROR tej grupy. Nie odczytywano poświadczeń i nie przedstawia się lokalnie odtworzonego komunikatu jako dosłownego cytatu z pełnego logu CI.

## Potwierdzony błąd instalatora

Odtworzono parametry generowane przez akcję dla instalacji Qt:

```text
python -m aqt install-qt windows desktop 6.12.0 win64_mingw --modules qtmultimedia --dry-run
```

Na Windows x64, Python **3.12.10**, aqtinstall **3.3.0**, py7zr **1.1.0**, wynik to exit **1**:

```text
WARNING : Failed to download checksum for the file 'online/qtsdkrepository/windows_x86/desktop/qt6_6120/qt6_6120/Updates.xml'.
ERROR   : Failed to locate XML data for Qt version '6.12.0'.
```

Przyczyną tego odtworzonego błędu jest wybór nieistniejącego repozytorium przez aqt 3.3.0. W jego `QtArchives._arch_ext()` wywoływane jest mapowanie `extension_for_arch()` rozróżniające m.in. Android/WASM, ale bez właściwego sufiksu Windows MinGW dla układu repozytoriów Qt >=6.8. Powstaje ścieżka `qt6_6120/qt6_6120/Updates.xml`; właściwa to:

```text
qt6_6120/qt6_6120_mingw/Updates.xml
```

W [oficjalnej rewizji aqt](https://github.com/miurahr/aqtinstall/commit/5a49f2b391ae8ee1d3c0dd87f8b33c1d49a4deb7) mapowanie uwzględnia wersję Qt i wariant platformy. Tę rewizję, raportującą `3.3.1.dev168`, sprawdzono lokalnie: identyczny dry-run Qt daje exit **0** i poprawne URL pakietów `_mingw`.

Błąd występuje także na Pythonie 3.12, więc **nie potwierdzono, że Python 3.15 jest pierwotną przyczyną #13**. Przypięcie Pythona zapewnia odtwarzalne warunki, ale samo nie naprawia aqt 3.3.0. Odtworzone niepowodzenie odpowiada konfiguracji i fazie joba #13; bez pełnego wnętrza logu nie można wykluczyć dodatkowego błędu specyficznego dla tego przebiegu.

## Dostępność i wybór pakietów

Pobrano metadane bez pobierania dużych archiwów Qt:

| Składnik | Potwierdzenie |
| --- | --- |
| Qt 6.12.0 / Windows MinGW x64 | [Oficjalne Updates.xml](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6120/qt6_6120_mingw/Updates.xml), pakiet `qt.qt6.6120.win64_mingw`, wersja `6.12.0-0-202609280346` |
| Qt Multimedia | W tych samych metadanych `qt.qt6.6120.addons.qtmultimedia.win64_mingw`, wersja `6.12.0-0-202609280346` |
| MinGW 15.1.0 x64 | [Osobne metadane tools_mingw1510](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/tools_mingw1510/Updates.xml), `qt.tools.win64_mingw1510`, wersja `15.1.0-202610271023` |

Zachowano poprawne wartości `version: 6.12.0`, `arch: win64_mingw`, `modules: qtmultimedia`. Parametr `tools: tools_mingw1510,qt.tools.win64_mingw1510` jawnie żąda osobnego kompilatora. Kod akcji zamienia przecinek na argumenty polecenia aqt install-tool; dokładnie te argumenty przeszły dry-run. Nie zakłada się, że instalacja samego Qt dostarcza kompilator.

Dry-run kompilatora wskazuje archiwum `15.1.0-202610271023mingw1510.7z` i katalog `Tools/mingw1510_64`. Ścieżka używana w workflow, `$QT_ROOT_DIR/../../Tools/mingw1510_64/bin`, jest zgodna z układem `Qt/6.12.0/mingw_64` oraz `Qt/Tools/mingw1510_64`.

Uwaga dotycząca metadanych dostawcy: pakiet bazowy Qt nadal deklaruje zależność `qt.tools.win64_mingw1310` i zawiera archiwum runtime nazwane 13.1.0. Nie jest to deklaracja zainstalowania wymaganego GCC 15.1. Dlatego zachowano osobny pakiet 15.1 i ścisłą kontrolę kompilatora. Nie modyfikowano ani nie mieszano lokalnych DLL. Rzeczywiste pliki runtime i deployment na świeżym runnerze musi potwierdzić pełny job, w tym istniejący krok deploy.ps1.

## Zmiany

Zmodyfikowano tylko `.github/workflows/qmake.yml` i dodano ten raport:

1. Osobny `actions/setup-python@v6` z **Python 3.12.10 x64** oraz kontrolą rzeczywistej wersji.
2. `setup-python: false` w install-qt-action, aby jego wewnętrzne `>=3.14.0` nie zastąpiło przypiętego interpretera.
3. `aqtsource` wskazuje oficjalne archiwum **konkretnego commita** `5a49f2b391ae8ee1d3c0dd87f8b33c1d49a4deb7`. Nie używa ruchomego master i nie patchuje lokalnie kodu instalatora. Jest to rewizja rozwojowa: publiczne PyPI podczas kontroli nadal wskazywało 3.3.0 jako najnowsze wydanie. Po wydaniu stabilnej wersji zawierającej poprawkę należy przejść na nią po weryfikacji.
4. **py7zr 1.1.3** — przypięta wersja zgodna z wymaganiem nowego aqt (`>=1.1.3`). Sprawdzono `pip check` oraz utworzenie i rozpakowanie małego archiwum 7z. Pozostawienie 1.1.0 powodowałoby konflikt zależności.
5. Zachowano sprawdzenie Qt 6.12.0 i GCC 15.1.0; dodano kontrolę `x86_64-w64-mingw32` i zapis użytych ścieżek do logu.
6. Artifact Windows zachowuje także `**/aqtinstall.log`, co umożliwi odczyt konkretnego błędu instalatora po kolejnej awarii.

Zachowano wszystkie kroki qmake, CMake, QtTest, CTest, deployment i dotychczasowe logi Windows. Job Linux porównano z HEAD: **identyczny**. Bez zmian produkcyjnego C++, bez obniżania Qt/GCC, bez continue-on-error i bez wyłączania testów.

## Wykonane kontrole

Wyniki są rzeczywiste; dry-run nie pobiera/nie rozpakowuje pakietów Qt:

| Kontrola | Wynik |
| --- | --- |
| aqt 3.3.0 + Python 3.12.10, Qt dry-run | FAIL, exit 1 — błędne repozytorium, jak wyżej |
| Przypięty aqt + Python 3.12.10 + py7zr 1.1.3, Qt dry-run | PASS, exit 0, 10 pakietów, w tym qtbase/qtsvg/qtmultimedia |
| Przypięty aqt, osobny MinGW dry-run | PASS, exit 0, 1 pakiet, `Tools/mingw1510_64` |
| Lista architektur Qt 6.12.0 | PASS, `win64_mingw` obecne |
| Lista modułów Qt 6.12.0 / win64_mingw | PASS, `qtmultimedia` obecne |
| `pip check` końcowych zależności | PASS, brak konfliktów |
| py7zr 1.1.3 — małe archiwum 7z z tekstem UTF-8 | PASS, zapis i odczyt identycznej treści |
| Lokalne qmake / GCC / target | 6.12.0 / 15.1.0 / x86_64-w64-mingw32 |
| actionlint 1.7.12 | PASS; bez niedostępnych shellcheck/pyflakes |
| `git diff --check` | PASS; Git jedynie ostrzega o przyszłej konwersji LF/CRLF |
| Kompilacja/testy aplikacji w tym zadaniu | Nie powtarzano — zmiany dotyczą instalatora CI, nie C++ ani lokalnego Qt |
| Pełna nowa instalacja Qt/GCC i Windows workflow po zmianie | **Niewykonane; oczekuje na GitHub Actions** |

Środowisko prób utworzono w katalogu tymczasowym. Przy instalowaniu źródłowego aqt w embeddable Python potrzebne było lokalne doinstalowanie narzędzi setuptools/wheel/setuptools-scm oraz `--no-build-isolation` (ograniczenie tego tymczasowego interpretera). Tych obejść nie przenoszono do workflow korzystającego z normalnego actions/setup-python. Pełną instalację zależności akcji na świeżym runnerze nadal trzeba potwierdzić.

Dowody: `C:\Users\gkedzior\AppData\Local\Temp\TuneScape-windows-ci13`. Kluczowe pliki: `jobs.json`, `annotations.json`, `action-run13.yml`, `qt6120-mingw.xml`, `mingw1510.xml`, `qt-dry-run.log`, `aqt-metadata-debug.log`, `qt-final-dry-run.log`, `tool-final-dry-run.log`, `list-arch-upstream.log`, `list-modules-upstream.log`, `aqt-upstream-install-retry.log`, `py7zr-install.log`, `actionlint-final.txt`. Log aqt wygenerowany podczas pierwszej próby w repozytorium przeniesiono do tego katalogu, zachowując dowód. Nie zmieniono instalacji C:\Qt ani globalnego Pythona.

## Co wymaga GitHub Actions

Pełny Windows job musi potwierdzić instalację źródłowego aqt na normalnym Pythonie 3.12.10, pobranie i kontrolę sum archiwów Qt, rozpakowanie, instalację osobnego MinGW 15.1, wersje/ścieżki, budowanie qmake/CMake, QtTest/CTest oraz deployment i runtime DLL. Dry-run potwierdza rozwiązanie pakietów, nie wykonanie tych operacji.

Workflow ma push na refactor/codebase-audit i pull_request, bez workflow_dispatch. Nie wykonano commit, push, merge ani rerun starego przebiegu, który nie zawierałby lokalnych zmian. Po autoryzowanym opublikowaniu zmian nowy przebieg powinien przejść oba joby; w razie błędu należy odczytać artifact windows-test-logs/aqtinstall.log. **Nie deklaruje się sukcesu Windows CI.**
