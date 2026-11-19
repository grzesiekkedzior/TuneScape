# Windows: Qt 6.12.0 / MinGW 15.1

qmake remains primary. Keep Qt 6.7.0 with GCC 11.2 in a separate kit/build directory for historical reproduction. Do not replace individual FFmpeg DLLs or mix compiler runtimes.

Install Qt 6.12.0 MinGW x64, Qt Multimedia and MinGW 15.1. Qt SVG, Widgets, Network, Concurrent and Test are supplied by the desktop base package. Verify that Qt SVG is actually present. Check qmake and g++ versions; a Qt directory named mingw_64 contains Qt libraries and does not itself establish the presence of the compiler.

For an existing verified official MinGW archive, verify the SHA-256 recorded in `TuneScapeTests/audio_diagnostics/stage5e-comparison.json`, extract into a new permanent tools directory, and check gcc, g++, mingw32-make and gdb. Never extract over another compiler. Qt Creator: register that gcc/g++, its GDB, and the Qt 6.12 qmake in a dedicated Desktop kit. Select that kit in the project; do not infer its compiler from its display name. No global PATH changes are needed.

Run PowerShell 7 from the repository root. Set the following paths to your installation; build and staging paths should be new empty directories outside the source tree, without `#` in their names.

```powershell
$repo = (Get-Location).Path
$qt = 'C:/Qt/6.12.0/mingw_64'
$cc = 'C:/Qt/Tools/mingw1510_64/bin'
$build = Join-Path $env:TEMP ('TuneScape-' + [guid]::NewGuid().ToString('N'))
$cmake = (Get-Command cmake.exe).Source
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
$ninja = 'C:/Qt/Tools/Ninja/ninja.exe'
$env:PATH = "$qt/bin;$cc;$env:SystemRoot/System32;$env:SystemRoot"
$env:QT_PLUGIN_PATH = "$qt/plugins"
$env:QT_QPA_PLATFORM = 'offscreen'
& "$qt/bin/qmake.exe" -query QT_VERSION # 6.12.0
& "$cc/g++.exe" -dumpfullversion       # 15.1.0
New-Item -ItemType Directory -Path "$build/qmake" | Out-Null
Set-Location "$build/qmake"
& "$qt/bin/qmake.exe" "$repo/TuneScape.pro" CONFIG+=release
if ($LASTEXITCODE) { throw 'qmake failed' }
& "$cc/mingw32-make.exe" -j4
if ($LASTEXITCODE) { throw 'Build failed' }
$p = Start-Process "$build/qmake/TuneScapeTests/release/TuneScapeTests.exe" -WindowStyle Hidden -PassThru -ArgumentList '-o',"$build/tests.txt,txt"
$p.WaitForExit()
if ($p.ExitCode) { throw 'QtTest failed' }
& $cmake -S $repo -B "$build/cmake" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$qt" "-DCMAKE_CXX_COMPILER=$cc/g++.exe" "-DCMAKE_MAKE_PROGRAM=$ninja"
if ($LASTEXITCODE) { throw 'Configure failed' }
& $cmake --build "$build/cmake" --parallel 4
if ($LASTEXITCODE) { throw 'Build failed' }
& $ctest --test-dir "$build/cmake" --output-on-failure --no-tests=error
if ($LASTEXITCODE) { throw 'CTest failed' }
```

Ninja and CMake must be installed; supply CMAKE_MAKE_PROGRAM if Ninja is outside PATH. Default tests use local HTTP and temporary data. Existing backend-dependent tests remain opt-in through `TUNESCAPE_TEST_MEDIA=1`; enable them on a machine with functioning audio. CI runs the default suite without an audio-device requirement. Public streams require separate explicit URLs and are not part of CI.

Deployment:

```powershell
& "$repo/tools/windows/deploy.ps1" -QtRoot $qt -CompilerBin $cc -Executable "$build/qmake/TuneScapeCore/release/TuneScapeCore.exe" -OutputDirectory "$build/dist"
& "$repo/tools/windows/smoke.ps1" -Distribution "$build/dist" -ResultsDirectory "$build/smoke"
```

The deploy script runs the selected windeployqt, uses qt.conf instead of patching Qt DLLs, validates DLL provenance by SHA-256, and writes deployment-manifest.json. FFmpeg runtime dependencies come only from the selected SDK. Embedded resources are compiled through the shared source manifest. Do not distribute tests, toolchains or debug tools. This creates a staging directory, not an installer or published release.

The smoke script opens our main window, checks its title and loaded qwindows/Qt DLL paths with developer PATH entries removed, then requests normal close. It uses an isolated working directory. It is intended for a desktop session, not the default headless CI suite. Passing on a developer machine does not qualify a clean Windows/VM deployment. Also validate actual playback, recording, TLS and long sessions before release.

Linux remains a separate native build with its own compatible Qt/FFmpeg/compiler. See `TuneScapeCore/BuildHelp.txt`; do not use Windows DLLs or infer Linux success from Windows tests. CI retains the Ubuntu job independently.
