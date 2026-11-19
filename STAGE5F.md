# TuneScape — Etap 5F

Raport z 19 listopada 2026, baza `8fd4b895`, gałąź `refactor/codebase-audit`.

**Status: lokalna konfiguracja i kontrolny deployment ukończone; etap częściowo ukończony do czasu potwierdzenia nowego workflow przez GitHub Actions.** Nie wykonano push ani zdalnego uruchomienia zmienionego workflow. Nie deklaruje się zaliczenia CI na podstawie lokalnych testów. Czysty Windows/VM i Linux pozostają nieweryfikowane.

## Środowisko i trwały kompilator

Qt 6.12.0 jest w `C:/Qt/6.12.0/mingw_64`; Multimedia i SVG są obecne. Użyto qmake 3.1/Qt 6.12.0, C++17 i GCC 15.1.0 x64. Rzeczywisty backend w probe dystrybucji zgłosił FFmpeg 9.0.1. Qt 6.7.0 i GCC 11.2 pozostawiono bez zmian; nadal można wybierać je osobno dla historycznego builda.

Nie znaleziono trwałego GCC 15.1. Zweryfikowano oficjalne archiwum użyte w 5E: SHA-256 `23250E2BB68D8E9A54170975D0027A405175C7ED01B397FFE72E2E89BFE00ED3`. Rozpakowano je do nowego `C:/Qt/Tools/mingw1510_64`. Nie nadpisano innych narzędzi i nie zmieniono globalnego PATH. gcc i g++ zgłosiły 15.1.0, Make 4.4.1, GDB 16.2. Kompilator nie zależy już od TEMP.

Odczyt konfiguracji Qt Creator ujawnił zarejestrowane Qt 6.12, ale jego Kit wskazuje kompilatory i debugger `mingw1310`. Ten Kit wymaga ręcznego wybrania GCC/GDB z nowego trwałego katalogu. Aktywnego wyboru projektu nie utożsamiono z samym wpisem Kitu. Ustawień globalnych nie zmieniano. Powtarzalny CLI nie korzysta z błędnego Kitu.

Przenośna instrukcja w `tools/windows/README.md` obejmuje instalację, konfigurację Kitu, qmake, CMake, testy, deployment i ograniczenia. `TuneScapeCore/BuildHelp.txt` wskazuje nową domyślną konfigurację Windows i zachowuje historyczną. Nie wprowadzono bezwzględnych ścieżek użytkownika do plików projektu. Lista źródeł, uic, C++17, qmake i CMake pozostały bez zmian.

## CI

Jedyny workflow znaleziony w repozytorium to `.github/workflows/qmake.yml`. Poprzedni Windows job deklarował Qt 6.7/MinGW 11.2, ale instalował narzędzie 9.0. Skorygowano go do jawnego Qt 6.12.0/MinGW 15.1.0 przez install-qt-action@v4 oraz `tools_mingw1510,qt.tools.win64_mingw1510`. Wersje qmake i kompilatora są sprawdzane; bin wybranego kompilatora otrzymuje pierwszeństwo w PATH joba.

Przed zmianą sprawdzono obsługę parametrów version/arch/modules/tools w dokumentacji akcji oraz oficjalne metadane Qt 6.12/Multimedia i MinGW 15.1. Qt SVG jest w bazowym pakiecie desktop, co potwierdzono w DownloadableArchives; Multimedia jest jawnie instalowanym modułem. Metadane pakietów są dostępne, lecz nie zastępują rzeczywistego wykonania akcji instalacyjnej na runnerze.

Windows buduje qmake i CMake, uruchamia QtTest oraz CTest, kontroluje staging i zachowuje logi/manifest jako artifact. Poprawiono oczekiwanie na zakończenie GUI exe QtTest — PowerShell czeka i propaguje rzeczywisty exit code. Domyślny job zachowuje istniejący kontrakt testów: backend audio jest opt-in, publiczne stacje są oddzielne. Testów nie usunięto. Lokalne testy z audio wykonano dodatkowo. Job Linux pozostał identyczny po normalizacji końców linii. Gałąź push i pull_request zachowano.

Parser PyYAML potwierdził poprawność składni i strukturę dwóch jobów. To kontrola statyczna, a nie walidacja przez serwery GitHub. Windows runner pozostaje windows-2022; jego system i brak urządzenia audio różnią się od desktopowego Windows 11 z 5E. Nie nazywa się ich tym samym środowiskiem. Należy sprawdzić pierwszy przebieg instalacji Qt/toolchainu, generator Ninja, buildów i testów na GitHub. Nie uruchomiono go, respektując zakaz push/commit.

## Kompilacje i testy lokalne

Artefakty są w `C:/Users/gkedzior/AppData/Local/Temp/TuneScape-stage5f-f8129c60`, poza źródłami.

| Kontrola | Wynik |
| --- | --- |
| Świeży qmake Release, cała aplikacja i testy | PASS |
| Pełny QtTest, TUNESCAPE_TEST_MEDIA=1 | 84 PASS, 0 FAIL, 1 SKIP, 46,273 s |
| Świeży CMake Release, cała aplikacja i testy | PASS |
| CTest z lokalnym audio | 2/2 PASS, 76,42 s |
| Odtwarzanie, A → B → A, Pause/Resume/Stop, nagrywanie i zamykanie HTTP | Zachowane testy integracyjne zaliczone w pełnym QtTest |
| Składnia PowerShell, YAML, git diff --check | PASS |
| Pełna macierz 144 procesów | Nie powtarzano — backend i biblioteki 5E nie zostały zmienione |
| Test publicznych stacji | Jawnie pominięty |
| Build Linux | Niewykonany; bieżący kontekst WSL zwraca WSL_E_DISTRO_NOT_FOUND dla Ubuntu-24.04 |

Pierwszą próbę budowania w izolowanym TEMP narzędzi przerwano po problemie z katalogiem zawierającym `#` i niedostępnością artefaktów między izolowanymi wywołaniami. Nie uznano jej za zaliczony build. Wszystkie wyniki z tabeli pochodzą z późniejszych świeżych buildów w stabilnej lokalizacji powyżej. Nie zmieniano kodu aplikacji, aby obejść problem środowiska narzędzi.

## Kontrolny deployment

Dodano parametryzowany `tools/windows/deploy.ps1`. Uruchomiono windeployqt z Qt 6.12.0, Release, compiler-runtime i bez tłumaczeń. Użyto `--no-patchqt` i lokalnego qt.conf, aby zachować weryfikowalne sumy oryginalnych DLL i relatywne ścieżki pluginów. Brakujące zależności FFmpeg mogą być kopiowane wyłącznie z wybranego SDK. Każda DLL jest porównywana SHA-256 z plikami tego SDK lub GCC 15.1. Odrzucane są stare nazwy bibliotek FFmpeg 6.1 i niewłaściwe wersje Qt. Niepustego katalogu staging nie nadpisuje się.

Payload `dist/` ma 31 plików, 96080268 bajtów (około 91,6 MiB), plus dopisany plik deployment-manifest.json (32 pliki łącznie). Pełna lista payloadu, wersje i sumy są w `tools/windows/deployment-stage5f.json` oraz w stagingu. Zawiera:

- TuneScapeCore.exe, qt.conf; zasoby aplikacji są skompilowane w exe przez qrc.
- Qt6Core, Gui, Widgets, Network, Multimedia i Svg.
- avcodec-63, avformat-63, avutil-61, swresample-7, swscale-10 — FFmpeg 9.0.1.
- libgcc_s_seh-1, libstdc++-6, libwinpthread-1 — runtime zgodnego zestawu.
- platforms/qwindows; multimedia/ffmpegmediaplugin i windowsmediaplugin, dobrane przez windeployqt.
- iconengines/qsvgicon; imageformats/qgif, qico, qjpeg, qsvg; styles/qmodernwindowsstyle.
- generic/qtuiotouchplugin; networkinformation/qnetworklistmanager; tls/qcertonlybackend i qschannelbackend.
- D3Dcompiler_47 i opengl32sw, runtime grafiki dobrany przez windeployqt.

Nie ma Qt 6.7 DLL ani FFmpeg 6.1.1. Nie ma testów, Qt6Test, debuggerów, kompilatorów ani plików deweloperskich. Qt Concurrent nie wymagał osobnej DLL w wynikowym grafie zależności. Nie tworzono instalatora ani paczki do publikacji. Obecność dodatkowego pluginu Windows Multimedia nie jest potwierdzeniem jego kwalifikacji; probe faktycznie wybrał FFmpeg.

## Uruchomienie z ograniczonym środowiskiem

`tools/windows/smoke.ps1` uruchomił skopiowany exe z odrębnym katalogiem danych i PATH zawierającym wyłącznie Windows/System32 oraz Windows. Usunięto odziedziczone ustawienia QT_*. Powstało okno o tytule TuneScape. Sprawdzono rzeczywiste ścieżki załadowanych Qt DLL, runtime GCC i qwindows: wszystkie były w dist. Wysłano normalne WM_CLOSE; proces zakończył się kodem 0, bez wymuszenia zakończenia. Zachowano smoke.json, listę modułów oraz stderr z diagnostyką pluginów.

Ponieważ samo otwarcie okna nie musi inicjować dekodera, dodatkowo wykonano lokalny probe istniejącym reproduktorem z DLL dystrybucji. Tylko exe diagnostyczny i Qt6Test.dll były w osobnym katalogu probe, poza dist; nie są częścią dystrybucji. PATH zawierał dist i katalogi systemowe, bez SDK/MinGW. Probe potwierdził Qt 6.12.0, FFmpeg 9.0.1 z dist, udane odtwarzanie z lokalnego HTTP, końcowe zero odtwarzaczy, połączeń i spóźnionych callbacków. Nie użyto publicznej stacji.

To rzeczywisty test ograniczonego środowiska na komputerze developerskim. Nie jest pełnym testem niezależności na czystym Windows: systemowe składniki, sterowniki i inne globalnie dostępne zależności nadal mogą mieć znaczenie. Nie wykonano VM, testów TLS publicznych stacji, pełnej wizualnej kwalifikacji GUI ani długotrwałego odsłuchu. Dane użytkownika i istniejące nagrania nie były używane.

## Dalsze działania i ocena migracji

Lokalny Windows ma teraz trwały, odtwarzalny toolchain i zweryfikowany kontrolny deployment. Zmiany CI są przygotowane i statycznie sprawdzone. Do uznania całego 5F za potwierdzony pozostaje rzeczywisty przebieg nowego workflow na GitHub, po wykonaniu publikacji zmian przez użytkownika w osobnym kroku. W tym zadaniu nie wykonano commit, push ani merge i nie zmieniono gałęzi.

Przed wydaniem należy dodatkowo przetestować staging na czystym Windows/VM, natywny Linux i jego deployment, publiczne HTTP/HTTPS oraz długie sesje. Ręcznie poprawić/utworzyć Kit Qt Creator 6.12/GCC 15.1; obecny błędny wpis 13.1 nie wpływał na jawne buildy CLI. Udokumentowano te czynności bez zmiany ustawień globalnych.

Zmieniono wyłącznie workflow Windows, dokumentację BuildHelp, dodano narzędzia staging/smoke, ich instrukcję, tekstowy manifest oraz ten raport. Logika odtwarzania, Recorder/Reader, API, Favorites/M3U, ikony i GUI pozostały bez zmian. Wcześniejsze raporty i dane 5E/5D zachowano.
