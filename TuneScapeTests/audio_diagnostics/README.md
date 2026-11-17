# TuneScape — diagnostyka etapu 5C

Raport z 17 listopada 2026. Bazą zmian jest `2f47727` na gałęzi `refactor/codebase-audit`.

Potwierdzono zależność HTTP 403 od identyfikacji klienta dla badanego adresu SomaFM. Jawny `User-Agent: TuneScape/6.0` umożliwił pobranie audio. Minimalny program audio odtworzył natywną awarię mono poza kodem TuneScape. Przyczyna tej awarii pozostaje nieustalona.

## Środowisko i wersje

| Element | Zmierzona wersja |
| --- | --- |
| Windows | Windows 11, x86_64 |
| Qt i Qt Multimedia | 6.7.0 |
| ffmpegmediaplugin.dll | 6.7.0.0 |
| Kompilator | MinGW GCC 11.2.0 |
| FFmpeg, av_version_info() | 6.1.1 |
| avcodec-60.dll | 60.31.102 |
| avformat-60.dll | 60.16.100 |
| avutil-58.dll | 58.29.100 |
| swresample-4.dll | 4.12.100 |
| OpenSSL użyty do budowy Qt Network | 3.0.7 |
| OpenSSL załadowany przez Qt Network | 3.6.5 |
| Wyjście w porównaniu pod GDB | Realtek, stereo, preferowane 48000 Hz |

SHA-256 pluginu multimediów to `CE805CD10B3BACC23812F296EBC34A16894F9144E66246842A1A8685E0DAF060`. SHA-256 `swresample-4.dll` to `3ED5E6F65013CC75821F4647BCABC7AA56B43CE299AFECB3D0F58057C32A14F6`.

W katalogu instalacji dostępny był tylko zestaw Qt 6.7.0. Nie wykonano porównania z nowszym Qt/FFmpeg. WSL Ubuntu 24.04 nie ma pakietów qmake6, qt6-base-dev, qt6-multimedia-dev, qt6-svg-dev, libqt6multimedia6 ani libavcodec60. Linux pozostaje nieweryfikowany.

## Reproduktor i próby audio

`main.cpp` w tym katalogu buduje samodzielny `TuneScapeAudioDiagnostics`. Korzysta bezpośrednio z Qt Multimedia, lokalnego HTTP i generowanych ramek ciszy. Nie zawiera klas TuneScape, kontrolera sesji, widoków ani kodu nagrywania.

Materiał mono ma nagłówek MPEG-1 Layer II, 44100 Hz, 128 kbps, jeden kanał. Materiał stereo ma nagłówek MPEG-1 Layer III, 44100 Hz, 128 kbps. Każda ramka ma 417 bajtów, a porcja zawiera 240 ramek. Dekoder Qt w udanych próbach mono zwrócił 276480 ramek PCM, jeden kanał i 44100 Hz.

Worker tworzy współdzielony QAudioOutput, wymienia QMediaPlayer, uruchamia Play i Stop, sprawdza postęp pozycji oraz powtarza sekwencję odpowiadającą Pause/Resume radia z etapu 5A. W co czwartym cyklu wykonuje szybkie B → A. Oczekuje na rzeczywiste usunięcie odtwarzaczy i zamknięcie połączeń. Na końcu niszczy aktywnie dekodujący odtwarzacz. Warianty Connecting i Buffering są dostępne osobno. W wariancie Buffering odtwarzacz może dojść do BufferedMedia przed kolejnym sprawdzeniem stanu. Ścisłe Stop podczas stanu Buffering sprawdza dodatkowy deterministyczny test kontrolera.

| Zestaw pomiarowy | Próby | Cykle na próbę | Awarie natywne |
| --- | --- | --- | --- |
| CMake RelWithDebInfo, mono, standardowe zatrzymanie | 12 | 8 | 1 |
| CMake RelWithDebInfo, stereo, standardowe zatrzymanie | 12 | 8 | 0 |
| qmake Release, mono, standardowe zatrzymanie | 5 | 12 | 1 |
| qmake Release, mono, bez odpinania wyjścia i czyszczenia źródła | 5 | 12 | 1 |
| qmake Release, mono, usunięcie poprzednika przed następnym utworzeniem | 5 | 12 | 0 |
| GDB, mono, standardowe zatrzymanie | 15 | 12 | 0 |
| GDB, stereo, standardowe zatrzymanie | 3 | 12 | 0 |
| GDB, dodatkowa próba mono | 1 | 30 | 0 |
| GDB, mono, _NO_DEBUG_HEAP=1 | 3 | 30 | 0 |
| GDB, samo QAudioDecoder, mono | 3 | samo dekodowanie pliku | 0 |
| qmake Release, stereo, pomiar pamięci | 1 | 100 | 0 |
| Windows Media Foundation, mono | 3 | zatrzymane w pierwszym cyklu | 0, lecz wszystkie zwróciły Unsupported media type |

Wstępne uruchomienie trybu samego QAudioDecoder mono zakończyło się także kodem `0xC0000005`. Dla tej próby nie uzyskano stosu. Trzy kolejne próby pod debuggerem przeszły. W końcowym CTest proces QAudioDecoder mono ponownie zakończył się jako CrashExit. W tym trybie nie ma QMediaPlayer, QAudioOutput ani połączeń HTTP. Brak stosu tej awarii nie pozwala potwierdzić, że jest identyczna z awarią workera. Pomiarów z tabeli nie należy sumować z wczesnymi próbami rozpoznawczymi, próbami z 5B ani jednostkowymi uruchomieniami CTest.

Mono worker w pierwszej macierzy uległ awarii w próbie 5, podczas cyklu 6, po osiągnięciu BufferedMedia. Program zakończył się kodem `0xC0000005`. Materiał stereo przeszedł wszystkie odpowiadające mu próby.

Bezpośrednie wywołania swr_convert(), na wyrównanych buforach i z tymi samymi DLL, przeszły po 10000 iteracji dla każdej z czterech kombinacji. Były to mono → mono i mono → stereo, przy częstotliwości wyjściowej 44100 oraz 48000 Hz. To próba kontrolna ograniczonego zakresu parametrów i buforów.

## Stos i klasyfikacja przyczyny

Stos z 5B obejmował swresample-4.dll, swr_convert(), ffmpegmediaplugin.dll i pętlę zdarzeń wątku Qt z QTimer::timerEvent() oraz QThread::exec(). Część nazw ramek, takich jak swresample_license(), pochodziła z symboli eksportowanych DLL. Brak pełnych symboli bibliotek ogranicza dokładność nazw i interpretacji stosu.

W powtarzanych próbach 5C pod GDB awaria nie wystąpiła. Udane uruchomienia pod debuggerem nie dowodzą naprawy problemu. Odtworzenie awarii w samodzielnym programie ogranicza zależność od GUI, katalogu stacji, nagrywania i logiki PlaybackController.

| Możliwa przyczyna | Ocena |
| --- | --- |
| Flagi lub indeksy stacji w aplikacji | Nie są potrzebne do odtworzenia awarii minimalnego programu |
| Cykl życia QMediaPlayer/QAudioOutput | Nadal możliwy udział w backendzie. Sama zmiana sekwencji Stop nie usunęła awarii |
| Qt Multimedia i jego użycie resamplera | Hipoteza zgodna z zaobserwowaną ścieżką, bez dowodu konkretnego błędu w kodzie Qt |
| FFmpeg | Obecność DLL w stosie jest dowodem miejsca wystąpienia awarii, a nie dowodem winy biblioteki. Bezpośrednie konwersje kontrolne przeszły |
| Format mono MPEG Layer II lub dane fixture'a | Potwierdzona korelacja w tych próbach. Wynik nie dowodzi awarii wszystkich rzeczywistych stacji mono |
| Ostateczna przyczyna | Nieustalona, ograniczona diagnostycznie do opisanej konfiguracji i toru audio |

Zestaw Windows Media Foundation odrzucił fixture mono. Przełączenie aplikacji na ten backend nie ma więc potwierdzenia jako skuteczna alternatywa. Nie zmieniono globalnego backendu.

## Zasoby i zamykanie

W udanych próbach worker raportował zero pozostawionych QMediaPlayer, zero połączeń HTTP i zero callbacków poprzedniej próby. Jedyny QAudioOutput żyje przez czas działania workera. Logi zawierają osobne zdarzenia niszczenia odtwarzaczy i wyjścia audio.

W próbie 100 cykli stereo private bytes wynosiły 28155904 przed startem, 29761536 przy cyklu 10, 30437376 przy cyklu 50, 30572544 przy cyklu 99 i 30429184 po zakończeniu. Po rozgrzaniu przyrost był mniejszy, a końcowa liczba graczy i połączeń wynosiła zero. Nie wykonano profilowania wszystkich alokacji natywnych i nie wykluczono wszystkich wycieków. Wzrost pamięci obejmuje również cache i alokacje backendu.

Nowy test zamykania RadioAudioManager wykazał kolejność niszczenia output → player w kodzie wejściowym. Wynikała z kolejności dzieci QObject oraz pozostawienia deleteLater() w kolejce. Destruktor kończy teraz usuwanie dzieci QMediaPlayer przed usunięciem współdzielonego QAudioOutput. Test wymaga kolejności player → output i usunięcia aktywnego połączenia. Związek tego uporządkowania z awarią mono pozostaje niepotwierdzony.

## HTTP 403

Próby dotyczyły dokładnie adresu `https://ice6.somafm.com/gsclassic-128-mp3`. Przechwycenie lokalnego HTTP ujawniło następujące żądania.

| Pole | Qt Multimedia / FFmpeg | QNetworkAccessManager bez jawnych nagłówków |
| --- | --- | --- |
| Protokół lokalnego żądania | HTTP/1.1 | HTTP/1.1 |
| User-Agent | Lavf/60.16.100 | Mozilla/5.0 |
| Accept | */* | brak jawnego pola |
| Icy-MetaData | 1 | brak |
| Range | bytes=0- | brak |
| Connection | close | Keep-Alive |
| Accept-Encoding | brak w przechwyceniu | gzip, deflate |

Domyślne Mozilla/5.0 jest zachowaniem zaobserwowanym w Qt Network. Aplikacja wcześniej nie ustawiała własnego User-Agent dla nagrywania.

W głównym porównaniu HTTPS wyłączono proxy dla QNetworkAccessManager, zgodnie z konfiguracją testów 5B.

| Wariant HTTPS | Powtórzenia | Wynik |
| --- | --- | --- |
| Domyślne nagłówki Qt Network | 2 | 403, 403 |
| User-Agent TuneScape/6.0 | 2 | 200, 200 i odebrane audio |
| Tylko Accept */* | 2 | 403, 403 |
| Tylko Icy-MetaData 1 | 2 | 403, 403 |
| TuneScape/6.0 i Accept */* | 2 | 200, 200 i odebrane audio |

Ostateczny URL pozostał identyczny. W żadnej z tych prób nie wystąpiło przekierowanie. Udane i odrzucone odpowiedzi używały TLS 1.3 z TLS_AES_256_GCM_SHA384, bez błędów certyfikatu. QNetworkReply raportował brak użycia HTTP/2. Wcześniejsze porównanie HTTP również dało 403 dla wariantu domyślnego oraz 200 po jawnej identyfikacji klienta.

We wstępnym porównaniu narastających nagłówków jedna próba HTTPS z samą identyfikacją klienta zakończyła się błędem anulowania/timeoutu, bez kodu HTTP. Pozostałe dane i osobne powtórzenia z tabeli pozwoliły oddzielić ten przypadek od odmowy 403.

Wniosek przyczynowy dotyczy badanego adresu i okresu pomiaru. Odpowiedź serwera zależała od User-Agent. Nie ustalono wewnętrznych reguł dostawcy. Poprawka przedstawia prawdziwą nazwę TuneScape. Nie wprowadza podszywania się pod FFmpeg, obsługi cookies ani automatycznego ponawiania odmowy.

Po dodaniu User-Agent do StreamRecorder i StreamReader pełny test publiczny przeszedł. Obejmował Radio Paradise HTTP, SomaFM HTTPS, A → B → A, dwa Pause/Resume, przyrost zapisu podczas pauzy i finalizację plików przy zmianie stacji i Stop.

Osobne lokalne testy wymuszają HTTP 403 bezpośrednio i po 302. Oba wymagają pojedynczego sygnału błędu, stanu Idle, zera zapisanych bajtów, zachowania istniejącego nagrania oraz informacji o HTTP 403 w GUI. Treść HTML odpowiedzi nie trafia do pliku. Komunikat błędu rejestratora zawiera teraz kod i opis odpowiedzi HTTP.

## Weryfikacja i zakres zmian

Raport wejściowy 5B podawał dla pełnego zestawu lokalnego z backendem 81 zaliczeń i jedno pominięcie publicznej próby. Po zmianach dwa końcowe przebiegi qmake z TUNESCAPE_TEST_MEDIA=1 dały po 84 zaliczenia, zero niepowodzeń i jedno pominięcie publicznego testu. Test publiczny uruchomiony osobno przeszedł (3 zaliczenia z inicjalizacją i zamknięciem). Diagnostyka qmake dała osiem zaliczeń, obejmując sześć potomnych prób mono/stereo/Connecting/Buffering/dekodowania oraz inicjalizację i zamknięcie.

Świeży build Windows qmake wykonał się z Qt 6.7.0 i MinGW 11.2. Build CMake RelWithDebInfo również przeszedł. CTest uruchamia główny zestaw i osobny program diagnostyczny. Wcześniejszy przebieg z włączonym backendem przeszedł 2/2. Końcowy CTest z TUNESCAPE_TEST_MEDIA=1, po ostatniej zmianie kodu, dał **1/2**: TuneScapeTests przeszedł, TuneScapeAudioDiagnostics miał 7 zaliczeń i 1 niepowodzenie (mono-decoder, CrashExit). Pozostałe próby diagnostyczne wykonały się mimo awarii procesu potomnego. Wyniku CTest nie uznano za zaliczony. git diff --check przeszedł.

Nowe testy deterministyczne obejmują Stop podczas Buffering z ignorowaniem spóźnionych zdarzeń oraz HTTP 403 bezpośrednio i po przekierowaniu. Rozszerzony test rzeczywistego backendu sprawdza zamknięcie aktywnego dekodera i kolejność niszczenia player → output. Dotychczasowe testy wielokrotnych Pause/Resume, przełączania, przedwcześnie zakończonej odpowiedzi i odzyskiwania po błędzie pozostają zachowane.

Produkcja otrzymała jawne User-Agent w StreamRecorder i StreamReader, czytelniejszy błąd HTTP oraz uporządkowane zamknięcie odtwarzaczy w RadioAudioManager. Testy multimediów z 5B działają teraz w procesach potomnych. Natywny SIGSEGV kończy daną próbę i jest raportowany jako niepowodzenie, a pozostałe przypadki mogą się wykonać.

Zmiany dotyczą trzech wymienionych plików produkcyjnych, `tst_tunescapetests.cpp`, `TuneScape.pro`, `CMakeLists.txt` oraz tego katalogu diagnostycznego. Zastany raport PLAYBACK_INTEGRATION.md pozostawiono do dyspozycji użytkownika. Nie wykonano commita, push ani merge.

## Uruchomienie reproduktora

Wymagane są Qt Core, Gui, Network, Multimedia i Test. Program nie łączy się z publicznym internetem w trybie workera.

Samodzielny build qmake może zostać wykonany w osobnym katalogu.

```powershell
qmake.exe C:/QtApps/TuneScape/TuneScapeTests/audio_diagnostics/audio_diagnostics.pro CONFIG+=release
mingw32-make.exe -j4
$env:QT_QPA_PLATFORM = 'offscreen'
.\release\TuneScapeAudioDiagnostics.exe --worker --mono --cycles 12
.\release\TuneScapeAudioDiagnostics.exe --worker --cycles 12
```

Opcje workera obejmują `--buffering`, `--connecting`, `--stop-only`, `--destroy-first` oraz `--stereo-output`. Każdą próbę należy uruchamiać w osobnym procesie i zachować kod zakończenia. `--decode --mono` i `--decode` sprawdzają QAudioDecoder. `--resample` wykonuje bezpośrednią próbę ABI FFmpeg 6 na DLL z Windows.

Automatyczne uruchomienie programem QtTest izoluje ryzykowne przypadki.

```powershell
$env:TUNESCAPE_TEST_MEDIA = '1'
.\TuneScapeAudioDiagnostics.exe -o diagnostics.txt,txt
ctest --output-on-failure --no-tests=error
```

W buildzie qmake program diagnostyczny jest w `TuneScapeTests/audio_diagnostics/release`. W buildzie CMake jest w katalogu głównym builda. Domyślny CTest bez TUNESCAPE_TEST_MEDIA=1 pomija próby wymagające działającego backendu audio.

GDB należy uruchomić na pojedynczym workerze.

```powershell
gdb.exe --args .\TuneScapeAudioDiagnostics.exe --worker --mono --cycles 30
```

Po uruchomieniu w GDB można użyć poniższych poleceń.

```text
run
bt 18
info registers
x/12i $pc-16
```

`--http-local` zapisuje rzeczywiste nagłówki Qt Network na lokalnym serwerze. Publiczny tryb `--http URL --variant N --no-proxy` wykonuje żądania do podanego adresu i jest uruchamiany osobno od lokalnego CTest. Warianty 0–7 odpowiadają kolejno domyślnemu żądaniu, TuneScape, TuneScape z Accept audio/mpeg, dodatkowo ICY, dodatkowo HTTP/1, samemu Accept */*, samemu ICY oraz TuneScape z Accept */*.

## Ocena wydania

Warstwa nagrywania poprawnie obsługuje odmowę i w sprawdzonym przypadku SomaFM działa z jawną identyfikacją TuneScape. Awaria mono w Qt 6.7.0/FFmpeg 6.1.1 pozostaje ryzykiem wydaniowym. Zmiana porządku zamykania została zweryfikowana osobno, a sporadycznych awarii nie uznano za usunięte.

Najmniejszym następnym krokiem jest porównanie tego samego reproduktora na kompletnym nowszym zestawie Qt/FFmpeg i na innym urządzeniu audio. Warto zebrać pełne symbole lub dump procesu i porównać fixture mono z plikiem mono zakodowanym niezależnym enkoderem. Ewentualną zmianę wersji backendu należy poprzedzić tą samą macierzą i testem nagrywania. Izolacja procesów chroni wykonanie testów. Nie stanowi zabezpieczenia produkcyjnego przed awarią natywną.

Pełne wydanie wymaga ponadto prób Linux, ręcznego odsłuchu i fizycznego suspend/resume Windows. Etap diagnostyczny spełnia warunek ograniczenia i udokumentowania nieustalonej przyczyny, ale nie uzasadnia deklaracji usunięcia ryzyka mono.
