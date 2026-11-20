# Etap 7A — Tray, MiniPlayer i zarządzanie oknami

Baza `bc1c7111ca2b807ace6b3a3dbbf5e7522537ca42`, gałąź `refactor/codebase-audit`. Początkowe drzewo czyste. Przed zmianami odczytano [CI #15](https://github.com/grzesiekkedzior/TuneScape/actions/runs/44819313674): oba joby Windows/Linux zakończyły się SUCCESS. Audyt wykonano przed zmianami kodu. Implementacja i lokalna weryfikacja Windows zakończone 20.11.2026. Linux/Qt 6.4.2 oraz ręczne zachowanie natywnych pulpitów wymagają potwierdzenia; nie deklaruje się ich zaliczenia.

## Semantyka wejściowa

Nie istnieją ustawienia close-to-tray, minimize-to-tray ani continue-in-background. `application.properties` zawiera notifications i darktheme; pierwszy steruje powiadomieniami, drugi nie stanowi polityki okien. Nie dodaje się nowych preferencji ani nie zmienia ich znaczenia.

Start pokazuje MainWindow. OS-owa minimalizacja nie zmienia odtwarzania. Przycisk Tray jawnie ukrywa MainWindow bez Stop. Przycisk MiniPlayer pokazuje osobny QFrame i ukrywa MainWindow. Max w MiniPlayer robi operację odwrotną. MainWindow nie ma closeEvent/changeEvent, więc zwykłe zamknięcie korzysta z domyślnego lastWindowClosed Qt, a Stop następuje dopiero w destruktorze. Exit z Tray wywołuje QApplication::quit. Samo hide nie wywołuje lastWindowClosed.

## Audyt

| Obszar | Potwierdzony problem / przyczyna | Zamierzona poprawka |
| --- | --- | --- |
| Tray activation | Trigger jest ignorowany; tylko DoubleClick przełącza hide/show. Zminimalizowane okno nadal jest visible, więc DoubleClick może je dodatkowo ukryć | Trigger i DoubleClick wykonują identyczne, idempotentne przywrócenie; jawna akcja menu Show TuneScape |
| KDE/Wayland | Kod wymaga zdarzenia DoubleClick, którego dostarczenie zależy od integracji pulpitu. Brak środowiska do potwierdzenia konkretnego zdarzenia u użytkownika | Nie uzależniać odzyskania okna od DoubleClick, nie wymuszać fokusu/prywatnych protokołów |
| Przycisk Tray | Zawsze hide, bez isSystemTrayAvailable; brak fallbacku i odzyskania po utracie Tray | Brak Tray: minimalizacja do taskbara; utrata podczas ukrycia: przywrócenie |
| MainWindow close / Exit | Inne okno główne MiniPlayer lub panel mogą utrzymać aplikację; porządkowanie sesji dopiero w destruktorze | Jednoznaczne zatrzymanie i ukrycie pomocniczych okien przy końcowym close/Exit |
| MiniPlayer close | Domyślne close osobnego okna może zamknąć ostatnie widoczne okno zamiast wrócić do MainWindow | Close MiniPlayera przywraca istniejące MainWindow; przełączanie używa hide bez rekurencji close |
| Przywracanie / flagi | Główne show nie usuwa minimalizacji; showNormal może gubić maksymalizację; stay-on-top robi bezwarunkowe show | Jedna funkcja prezentacji, zachowanie stanu maksymalnego, stay-on-top nie otwiera ukrytego Mini |
| Geometria | Brak kontroli Mini po odłączeniu ekranu; panel oblicza qBound z nieprawidłowym zakresem dla małych ekranów; bezwarunkowy move na Wayland | Wspólny ograniczony prostokąt dla ekranów, reakcje na topologię, rezygnacja z move na Wayland |
| Czas życia | Tray przechowuje surowe Ui/MainWindow, a callbacki ikon przechwytują this i referencje do wektorów; akcje mogą żyć do destruktora bazowego | QPointer do potrzebnych widgetów, odłączenie przed usunięciem widoku, usunięcie odbiorców ikon przed polami Tray |
| Playback/UI | Jeden kontroler i współdzielone metadane działają; Mini jest pojedynczym członkiem RadioList | Zachować istniejące komponenty, bez nowego odtwarzacza i kopii stanu sesji |

Źródła platformowe: [QSystemTrayIcon](https://doc.qt.io/qt-6/qsystemtrayicon.html) opisuje różne ActivationReason i ograniczenia GNOME; [Qt Window and Dialog Widgets](https://doc.qt.io/qt-6/application-windows.html) opisuje brak gwarantowanego pozycjonowania zwykłych okien Wayland/XDG Shell. Przywracanie może pokazać i odminimizować okno oraz poprosić o aktywację; nie może zagwarantować fokusu kompozytora ani rogu ekranu na Wayland.

## Przyczyna zachowania Tray na KDE/Wayland

Potwierdzono błąd po stronie aplikacji: pomijała Trigger i polegała na DoubleClick. W oficjalnym [adapterze StatusNotifierItem Qt 6.4.2](https://github.com/qt/qtbase/blob/v6.4.2/src/gui/platform/unix/dbustray/qstatusnotifieritemadaptor.cpp) metoda Activate emituje Trigger, ContextMenu emituje Context, a SecondaryActivate emituje MiddleClick. Nie ma tam mapowania Activate na DoubleClick. Dotyczy to ścieżki SNI stosowanej m.in. przez KDE; sama obecność Wayland nie oznacza jednego kontraktu wszystkich hostów Tray. To potwierdza niezgodność oczekiwania aplikacji ze standardową ścieżką Qt, bez potrzeby instalacji ShellQt/KWin lub rozszerzeń.

Drugi niezależny błąd to sprawdzanie isVisible zamiast stanu minimalizacji i przełączanie hide/show. Zminimalizowane okno może być visible; dawna obsługa DoubleClick ukrywała je zamiast przywracać. Teraz Trigger i DoubleClick są idempotentną intencją „pokaż istniejące MainWindow”. Sekwencja Trigger → DoubleClick nie ukrywa przywróconego okna.

Nie zarejestrowano zdarzeń na rzeczywistym KDE użytkownika i nie przeprowadzono ręcznego testu kompozytora. Analiza źródeł Qt potwierdza przyczynę w kodzie obsługi zdarzeń, a nie gwarancję zachowania dowolnej wersji KDE/GNOME. [Release notes Qt 6.5](https://github.com/qt/qtreleasenotes/blob/dev/qt/6.5.0/release-note.md) opisują dodanie obsługi tokena xdg-activation w backendzie SNI; wersja Qt i host mogą dodatkowo wpływać na fokus. Aplikacja korzysta wyłącznie ze standardowych żądań Qt i zachowuje zgodność źródeł z Qt 6.4.2.

## Wykonane poprawki

### Wspólna prezentacja okien

Dodano mały zestaw funkcji WindowPresentation, bez nowego kontrolera odtwarzania i kopii stanu sesji. Restore usuwa tylko WindowMinimized, zachowując flagę maksymalizacji/fullscreen, pokazuje to samo QWidget, koryguje geometrię na obsługiwanych platformach i próbuje raise/activateWindow. Nie wywołuje Play/Stop i nie tworzy nowych okien.

Funkcja fittedGeometry jest niezależna od systemu okien: wybiera dostępny ekran o największej części wspólnej, przy braku części wspólnej pierwszy (w runtime najpierw ekran główny), ogranicza rozmiar i pozycję bez odwróconych zakresów qBound. Obsługuje ujemne współrzędne i brak listy ekranów. Dane runtime pochodzą z availableGeometry w logicznych współrzędnych Qt; nie mnoży się ich ręcznie przez DPR.

Na backendzie wayland/wayland-egl funkcja keepOnScreen nie żąda absolutnego move/resize. Kompozytor wybiera położenie. Także pozycjonowanie panelu Now Playing omija move na Wayland. Nie zmieniono Qt::Tool/WindowDoesNotAcceptFocus panelu, jego stylu ani układu.

### Tray i wyjście

- Dodano jawną akcję Show TuneScape, dostępną także wtedy, gdy pulpit nie generuje DoubleClick. Context nadal służy menu, a MiddleClick/Unknown nie wykonują sprzecznych operacji.
- Przycisk Tray ukrywa okno wyłącznie po dodatnim isSystemTrayAvailable. W przeciwnym razie minimalizuje do paska zadań i pokazuje krótki status wyjaśniający fallback. Nie jest wymagane doinstalowanie czegokolwiek.
- Gdy aplikacja została jawnie ukryta do Tray, timer co 1 s kontroluje dostępność hosta. Utrata Tray przywraca MainWindow; timer wyłącza się po przywróceniu lub wyjściu. Nie jest to ciągłe odpytywanie podczas zwykłej pracy z widocznym oknem.
- Restore z Tray pokazuje MainWindow i ukrywa MiniPlayer, bez zmiany sesji. Nie składa się żądań Play ani nie przełącza ponownie stacji.
- Exit zatrzymuje istniejący PlaybackController, ukrywa MiniPlayer, panel i ikonę, zamyka MainWindow oraz jawnie wywołuje QApplication::quit. Nie zależy od tego, które okno jest aktualnie widoczne. Powtórne żądanie Stop jest idempotentne zgodnie z istniejącym kontrolerem.
- MainWindow::closeEvent zachowuje semantykę Exit: kończy sesję i ukrywa pomocnicze okna, zanim Qt obsłuży standardowe zamknięcie. Nie zmieniono quitOnLastWindowClosed na false i nie dodano close-to-tray.

### MiniPlayer

Pozostał pojedynczym członkiem RadioList i korzysta z tego samego PlaybackController/PlayerUIController. Close MiniPlayera przywraca MainWindow, a Max/restore używa hide zamiast rekurencyjnego close. Nawigacja nie zatrzymuje ICY, nagrywania ani aktualizacji metadanych.

Stay on top zachowuje widoczność i stan minimalizacji; aktualizacja flagi ukrytego Mini nie pokazuje go przypadkowo. Istniejący QRadioButton otrzymał autoExclusive=false, aby można było także wyłączyć tryb, bez redesignu kontrolki. Mini reaguje na screenAdded/screenRemoved i availableGeometryChanged, z odroczoną kontrolą po usunięciu ekranu. Panel Now Playing reaguje analogicznie.

Naprawiono rozjazd głośności: dial Mini i główny slider są aktualizowane wspólnie z QSignalBlocker. Wcześniej ścieżka powrotu z Tray omijała kopiowanie dial → slider, a kolejne otwarcie Mini mogło przywrócić starą wartość. Zmiana okna nie powinna zmieniać głośności; nie dodano niezależnej kopii stanu odtwarzania.

### Czas życia i własność

Tray nie przechowuje surowego Ui pointera: potrzebne QLabel i MainWindow są QPointer. Zniszczenie centralwidget unieważnia powiązanie, ukrywa helpery i odłącza RadioList/PlaybackController. RadioList::detachView dodatkowo ukrywa i wyłącza Mini. Odłączony Mini nie otworzy usuniętego MainWindow.

Przy zmianie/usunięciu RadioList Tray odłącza stare sygnały i usuwa stare akcje stacji. Powtórne setRadioList z tym samym żywym obiektem pozostaje idempotentne. Callbacki ikon mają QPointer do właściciela, sprawdzają shutdown i aktualny wiersz/akcję; destruktor Tray usuwa menu/odbiorców zanim zniszczone zostaną przechwycone wektory. Odroczone pokazywanie panelu i akcje po shutdown są ignorowane.

MainWindow nadaje QObject parent istniejącym RadioList, Tray i Menu, zachowując dotychczasową jawną kolejność niszczenia usług. Widget Tray pozostaje ukrytym właścicielem helperów, nie dodatkowym panelem aplikacji. Panel Now Playing ma WA_QuitOnClose=false i jest jawnie ukrywany przy wyjściu.

Dodano opcjonalne parametry istniejących RadioBrowserApi/PlaybackController w konstruktorze MainWindow oraz funkcję sprawdzającą dostępność Tray w konstruktorze TrayIcon. Domyślne ścieżki produkcyjne są niezmienione; testy mogą używać prawdziwego MainWindow z localhost oraz deterministycznej dostępności Tray. Wstrzyknięte usługi są pożyczone i muszą żyć dłużej niż okno, tak jak istniejące opcjonalne zależności RadioList.

## Semantyka po zmianach

| Operacja | Widoczność | Sesja i nagrywanie |
| --- | --- | --- |
| Start | MainWindow, Mini ukryty | Bez automatycznej nowej sesji |
| Minimalizacja OS | Okno dostępne przez taskbar/overview | Bez Stop i zmiany generacji |
| Przycisk Tray, host dostępny | Main/Mini ukryte, ikona żądana widoczna | Kontynuacja w tle |
| Przycisk Tray, host niedostępny | Main zminimalizowane, nie ukryte | Kontynuacja, możliwość przywrócenia z taskbara |
| Utrata hosta po ukryciu | Main przywracane przez kontrolę dostępności | Ta sama sesja |
| Trigger / DoubleClick / Show TuneScape | To samo MainWindow, bez minimalizacji; Mini ukryty | Ta sama sesja i głośność |
| Otwarcie Mini | Ten sam Mini widoczny, Main ukryte | Ta sama sesja |
| Close lub Max Mini | Main przywrócone, Mini ukryty | Kontynuacja; metadane aktualizują nadal oba widoki |
| Close Main | Wszystkie pomocnicze okna i Tray ukryte, normalny Exit Qt | Stop, poprawne zakończenie nagrania |
| Exit z Tray | Jawne QApplication::quit, niezależnie od widoczności | Stop i zachowanie wcześniej zapisanych danych |

## Windows, X11, Wayland i GNOME

Windows i X11 używają standardowych żądań pozycji i aktywacji Qt. Po zmianach ekranów aplikacja ogranicza normalne okna do dostępnego obszaru; ostateczna realizacja żądań nadal należy do systemu okien. Zmiana rozmiaru nie obchodzi minimalnego rozmiaru layoutu: na ekranie mniejszym niż minimum można odzyskać położenie okna, ale nie zagwarantować pełnego zmieszczenia wszystkich kontrolek bez zmian GUI.

Wayland/XDG Shell nie daje gwarancji absolutnej pozycji zwykłego top-level. Program nie obiecuje Mini/panelu w konkretnym rogu i nie używa ShellQt, protokołów prywatnych, skryptów KWin ani manipulowania regułami kompozytora. Show i usunięcie żądanego stanu minimalizacji to obsługiwana intencja, a raise/activateWindow to próba aktywacji — fokus może pozostać w innym oknie.

GNOME bez integracji Tray nie wymaga rozszerzenia: aplikacja używa fallbacku do zwykłego minimalizowanego okna. Jeżeli host udostępnia Tray, korzysta się z obsługiwanych zdarzeń i akcji menu. isSystemTrayAvailable jest oceną Qt, nie dowodem, że wadliwy host faktycznie narysował ikonę; nie da się naprawić niezgodnego hosta przez prywatne obejścia aplikacji.

## Testy

Dodano 17 przypadków danych w 7 funkcjach testowych:

| Test | Pokrycie |
| --- | --- |
| testTrayRestoresWindow (3) | Ukryte, zminimalizowane i max+min; wielokrotne Trigger/DoubleClick bez przełączania hide; menu restore, Context bez zmiany okna, jedno MainWindow, niezmieniona generacja i jeden start |
| testTrayAvailability (2) | Dostępny host i jego utrata (rzeczywisty timer), brak hosta od początku, background playback bez nowej sesji |
| testMiniPlayerWindowTransitions (1) | Main ↔ Mini, minimalizacja, Close/Max, wspólny Play/Pause/Stop i volume, ICY/ikona przy ukrytym Main, zmiana stacji, zamknięcie Mini w trakcie aktualizacji, dwukierunkowy Stay on top |
| testWindowObjectsLifetime (3) | Widgety przed kontrolerami, Tray przed RadioList, RadioList przed Tray; aktywne ICY/ikony, brak wywołań do usuniętych kontrolek i zwolnienie żądań ikon |
| testMainWindowCloseSemantics (1) | Prawdziwy MainWindow z lokalnym API; OS-minimize bez Stop, końcowy close ukrywa także Mini/Tray i kończy sesję |
| testTrayExitStopsAndQuits (1) | Osobny proces, prawdziwe MainWindow i QApplication::exec; Close Mini nie kończy aplikacji, Exit z ukrytego Main/aktywnego Mini kończy pętlę, zamyka HTTP i zapisuje 13 oczekiwanych bajtów nagrania |
| testWindowGeometry (6) | Zwykły ekran, usunięty monitor, ujemne współrzędne, mały obszar roboczy, drugi monitor, brak ekranów; osobna polityka Wayland |

Testy nie wymagają natywnego Tray ani prawa do fokusu. Sprawdzają intencje/stan QWidget i sesję, a nie potwierdzenie aktywacji przez kompozytor. Lokalny HTTP i QTemporaryDir izolują dane; kontrolowany test nagrywania jawnie zapisuje w katalogu tymczasowym, nie w Music użytkownika. Zachowano wszystkie testy 6B, PlaybackController, Recorder, Radio Browser, Favorites i P0.

W pierwszym uruchomieniu nowego testu lifetime zabrakło RadioExplorer/createMenu w fixture przed loadAllData, co spowodowało odwołanie do pustego modelu drzewa. Uzupełniono fixture, bez obchodzenia testu i bez zmian architektury RadioList. Zachowano log i ślad GDB. Po poprawce wszystkie nowe przypadki przeszły: 19/0/0 wraz z init/cleanup, 3,292 s; test procesu potomnego 3/0/0. Nie opisuje się tego pośredniego błędu jako potwierdzonej awarii niszczenia widgetów w produkcji.

## Weryfikacja

| Kontrola | Rzeczywisty wynik |
| --- | --- |
| Świeży standalone qmake, aplikacja | PASS; Qt 6.12.0 / MinGW 15.1, osobny katalog build |
| qmake, projekt z testami | PASS; aplikacja, QtTest i audio diagnostics zbudowane |
| Nowe testy okien | 19 PASS / 0 FAIL / 0 SKIP wraz z init/cleanup; 3,292 s |
| Pełny QtTest qmake, TUNESCAPE_TEST_MEDIA=0 | 128 PASS / 0 FAIL / 4 SKIP; 31,228 s |
| Pełny QtTest qmake, TUNESCAPE_TEST_MEDIA=1 | 131 PASS / 0 FAIL / 1 SKIP; 52,937 s |
| Świeży CMake/Ninja, BUILD_TESTING=ON | PASS; aplikacja i oba programy testowe |
| CTest, tryb domyślny | 2/2 programy PASS; 32,64 s. QtTest bez audio, diagnostyka ma 6 jawnych SKIP |
| CTest -R TuneScapeAudioDiagnostics, TUNESCAPE_TEST_MEDIA=1 | 1/1 program PASS; 8 PASS / 0 FAIL / 0 SKIP wraz z init/cleanup; 20,21 s |
| git diff --check | PASS |
| Linux/Qt 6.4.2, KDE/Wayland, GNOME, X11 | Niewykonane dla zmian 7A — brak lokalnego środowiska; wymagają późniejszego CI i testów ręcznych |

Historyczny wynik bazowy z raportu 6B to 114/0/1 z audio; po 7A pełny QtTest ma 17 dodatkowych przypadków danych. Zachowano istniejące testy. Jedyny SKIP w pełnym przebiegu z audio to test publicznych stacji (brak jawnych TUNESCAPE_LIVE_STREAM_A/B). Domyślny przebieg dodatkowo pomija trzy testy rzeczywistego backendu; nie liczy się ich jako wykonanych. Przebiegi domyślny i z audio oraz CTest ponownie sprawdziły nowe regresje okien.

Osobna diagnostyka audio uruchomiła sześć procesów: mono/stereo playing, mono buffering/connecting oraz mono/stereo decoder. Wszystkie zakończyły się kodem 0, bez CrashExit. Inwentarz każdego procesu potwierdza Qt/Qt Multimedia 6.12.0, GCC 15.1.0, plugin `ffmpegmediaplugin.dll` i biblioteki z `C:\Qt\6.12.0\mingw_64`, FFmpeg 9.0.1 (`avcodec/avformat 63.1.101`, `avutil 61.1.101`, `swresample 7.1.101`). Wykryto dostępne urządzenie stereo 48000 Hz. Są to kontrolowane lokalne próbki i testy cyklu życia backendu; nie jest to ręczny odsłuch ani dowód stabilności dowolnego strumienia MP2. Nie powtarzano historycznej macierzy 5D/5E i nie zmieniano jej wyników.

Środowisko Windows x64: Qt 6.12.0, GCC 15.1.0, C++17, GUI offscreen; buildy i logi poza repozytorium w `C:\Users\gkedzior\AppData\Local\Temp\TuneScape-stage7a-20261120`. Dowody: `core-qmake-*.log`, `qmake-*.log`, `fix-build.log`, `window-focused-fixed.txt`, `full-media-0.txt`, `full-media-1.txt`, `cmake-*.log`, `ctest-default.txt`, `ctest-audio.txt`, `ctest-audio-details.txt`. W logach offscreen występują oczekiwane ostrzeżenia o braku obsługi raise i istniejący błąd stylesheet Mini; nie są traktowane jako potwierdzenie natywnej aktywacji okna. CMake zgłasza opcjonalny brak WrapVulkanHeaders, ale konfiguracja i linkowanie kończą się poprawnie.

Linux lokalnie niewykonany: WSL zgłasza brak dystrybucji, nie znaleziono Docker/Podman. Nie instalowano nowego środowiska. Nowy kod używa API dostępnego w Qt 6.4.2, a wspólny manifest zawiera nowe pliki z poprawną wielkością liter; przegląd źródeł nie zastępuje kompilacji Linux. Baza 6B ma rzeczywisty PASS CI #15; nie jest to wynik nowych zmian 7A. Nie uruchamiano zdalnego CI, bo nie wykonywano push.

## Ręczne scenariusze do wykonania

Poniższe scenariusze **nie zostały wykonane ręcznie** w tym zadaniu:

1. Windows 11: uruchomić stację, zminimalizować i przywrócić z taskbara. Dźwięk/nagrywanie i bieżący tytuł mają trwać bez ponownego Play.
2. Windows/KDE: ukryć przez przycisk Tray, wykonać pojedynczy klik, dwuklik i kilka aktywacji. Ma pojawić się jedno okno; kolejne zdarzenie nie może go ponownie schować.
3. KDE Plasma/Wayland: przywrócić przez klik i przez Show TuneScape w menu, także po minimalizacji i z włączonym Mini. Oddzielnie zanotować widoczność, żądany stan normalny/max oraz przyznanie/odmowę fokusu przez kompozytor.
4. GNOME bez dodatkowych rozszerzeń: sprawdzić brak hosta Tray i fallback przycisku do taskbara/overview; użytkownik ma móc ponownie otworzyć aplikację standardowym przełączaniem okien.
5. Windows/X11/KDE: przełączać Main ↔ Mini, używać Close oraz Max Mini, zmieniać głośność i dwa razy przełączyć Stay on top. Zmieniać stację/metadane podczas ukrytego Main.
6. Windows i KDE/Wayland: przenieść Mini na drugi monitor, zmienić skalowanie/obszar roboczy, następnie odłączyć monitor. Windows/X11 mają próbować odzyskać geometrię na aktywnym ekranie; na Wayland położenie należy oceniać według decyzji kompozytora, bez wymagania wybranego rogu.
7. Sprawdzić panel Now Playing podczas menu oraz po przypięciu i odpięciu; nie ma blokować zakończenia aplikacji. Zweryfikować mały obszar roboczy i różne DPR bez zmiany QSS.
8. W testowej sesji pulpitu sprawdzić utratę dostępności hosta Tray podczas ukrycia — Main powinno powrócić po kontroli dostępności, bez restartu stacji.
9. Podczas nagrywania wykonać Exit z Tray, gdy widoczny jest tylko Mini; potwierdzić zakończenie procesu, zamknięcie strumieni i zachowanie zapisanych danych. Powtórzyć zwykły Close Main, także z otwartym About/panelem.
10. Powtórzyć podstawowe scenariusze na KDE/X11, jeśli dostępne. Nie przenosić wyniku jednego kompozytora na wszystkie pozostałe.

## Zmienione pliki i dalsze prace

- `TuneScapeCore/include/WindowPresentation.h`, `TuneScapeCore/src/WindowPresentation.cpp` — wspólne żądanie restore i geometria, bez zewnętrznych zależności.
- `TuneScapeCore/include/trayicon.h`, `TuneScapeCore/src/trayicon.cpp` — restore, dostępność/fallback, Quit, lifetime, geometria panelu.
- `TuneScapeCore/include/miniplayer.h`, `TuneScapeCore/src/miniplayer.cpp` — Close/Show, topologia ekranów i Stay on top.
- `TuneScapeCore/include/mainwindow.h`, `TuneScapeCore/src/mainwindow.cpp` — jawne końcowe close, własność i opcjonalne zależności testów.
- `TuneScapeCore/src/radiolist.cpp` — tylko przełączanie okien, odłączenie Mini i synchronizacja suwaków; bez zmian architektury list/sesji/metadanych.
- `TuneScapeCore/sources.list`, `TuneScapeTests/tst_tunescapetests.cpp`, `STAGE7A.md` — build, regresje i raport.

Nie zmieniono PlaybackController, MusicBrainz/ICY/JsonRequestQueue, StreamRecorder, RadioBrowserApi, Favorites/M3U, Country, QSS, workflow ani deploymentu. Nie wykonano commit/push/merge ani zmiany gałęzi. Nie modyfikowano danych użytkownika ani istniejących nagrań.

Do 7B pozostawić wygląd Mini i panelu, style oraz istniejące ostrzeżenie niepoprawnego stylesheet Mini (przekazany jako ścieżka). Nie naprawiano go przy okazji, bo etap wyklucza QSS. Ewentualny szerszy audyt własności historycznych popupów menu aplikacji powinien być osobnym zakresem; tutaj parent nadano wrapperowi Menu, bez przebudowy importu/eksportu. Przed wydaniem pozostają rzeczywisty CI dla 7A i powyższa macierz ręczna. Nie ma gwarancji fokusu, pozycji rogu ani natywnego renderowania Tray na każdym kompozytorze Wayland.
