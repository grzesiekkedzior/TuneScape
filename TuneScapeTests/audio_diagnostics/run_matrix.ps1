# Requires PowerShell 7. Each trial has its own process, log, timeout and optional crash dump.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$QtRoot,
    [Parameter(Mandatory)][string]$CompilerBin,
    [Parameter(Mandatory)][string]$FixturesDirectory,
    [Parameter(Mandatory)][string]$ResultsDirectory,
    [ValidateRange(1,100)][int]$Trials=12,
    [ValidateRange(1,200)][int]$Cycles=12,
    [ValidateRange(1,300)][int]$TimeoutSeconds=60,
    [string[]]$DeviceIds=@('default'),
    [string[]]$Names=@(),
    [ValidateSet('worker','decode')][string[]]$Modes=@('worker','decode'),
    [switch]$CaptureDump
)
$ErrorActionPreference = 'Stop'
$QtRoot = (Resolve-Path -LiteralPath $QtRoot).Path
$Executable = (Resolve-Path -LiteralPath $Executable).Path
$FixturesDirectory = (Resolve-Path -LiteralPath $FixturesDirectory).Path
$ResultsDirectory = [IO.Path]::GetFullPath($ResultsDirectory)
if ((Test-Path -LiteralPath $ResultsDirectory) -and
    (!(Test-Path -LiteralPath $ResultsDirectory -PathType Container) -or
     (Get-ChildItem -LiteralPath $ResultsDirectory -Force | Select-Object -First 1))) {
    throw 'Use a fresh, empty results directory; existing files are preserved'
}
New-Item -ItemType Directory -Path $ResultsDirectory -Force | Out-Null
foreach ($required in @('bin/qmake.exe','bin/Qt6Core.dll','bin/Qt6Multimedia.dll','plugins/multimedia/ffmpegmediaplugin.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $QtRoot $required))) { throw "Incomplete Qt kit: $required" }
}
$expectedQtVersion = (& (Join-Path $QtRoot 'bin/qmake.exe') -query QT_VERSION | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $expectedQtVersion -notmatch '^6\.\d+\.\d+$') { throw 'Cannot identify the selected Qt kit' }
$compilerExecutable=Join-Path $CompilerBin 'g++.exe'
$expectedCompiler=if (Test-Path -LiteralPath $compilerExecutable) { (& $compilerExecutable -dumpfullversion | Out-String).Trim() } else { '' }
if (Test-Path -LiteralPath (Join-Path (Split-Path $Executable) 'Qt6Core.dll')) {
    throw 'Use a clean build directory without deployed Qt DLLs beside the executable'
}
$fixtures = @(Get-Content -Raw -LiteralPath (Join-Path $FixturesDirectory 'fixtures.json') | ConvertFrom-Json)
if ($Names.Count) { $fixtures = @($fixtures | Where-Object Name -in $Names) }
if (!$fixtures.Count) { throw 'No matching fixtures' }
$kitFiles = @('bin/Qt6Core.dll','bin/Qt6Multimedia.dll','plugins/multimedia/ffmpegmediaplugin.dll')
$kitFiles += Get-ChildItem (Join-Path $QtRoot 'bin') -File | Where-Object Name -match '^(avcodec|avformat|avutil|swresample)-\d+\.dll$' | ForEach-Object { 'bin/' + $_.Name }
$kitFiles | ForEach-Object {
    $file = Get-Item -LiteralPath (Join-Path $QtRoot $_)
    [pscustomobject]@{File=$file.FullName; Version=$file.VersionInfo.FileVersion; Sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ResultsDirectory 'kit.json') -Encoding utf8
$results = [Collections.Generic.List[object]]::new()
foreach ($fixture in $fixtures) {
    $inputFile = Join-Path $FixturesDirectory $fixture.File
    if ((Get-FileHash -LiteralPath $inputFile -Algorithm SHA256).Hash -ne $fixture.Sha256) { throw 'Fixture hash mismatch' }
    foreach ($mode in $Modes) {
        $devices = if ($mode -eq 'worker') { $DeviceIds } else { @('none') }
        foreach ($device in $devices) {
            # Windows device IDs often share a long prefix. Hash the complete ID for file names.
            $deviceLabel = if ($device -in @('default','none')) { $device } else {
                [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData(
                    [Text.Encoding]::UTF8.GetBytes($device))).Substring(0,16)
            }
            for ($trial=1; $trial -le $Trials; ++$trial) {
                $name = "$($fixture.Name)-$mode-$deviceLabel-$trial"
                $dump = Join-Path $ResultsDirectory ($name + '.dmp')
                $info = [Diagnostics.ProcessStartInfo]::new($Executable)
                $info.UseShellExecute=$false; $info.CreateNoWindow=$true
                $info.RedirectStandardError=$true; $info.RedirectStandardOutput=$true
                $info.WorkingDirectory=$ResultsDirectory
                $info.Environment['PATH']="$(Join-Path $QtRoot 'bin');$CompilerBin;$env:SystemRoot\System32;$env:SystemRoot"
                $info.Environment['QT_PLUGIN_PATH']=Join-Path $QtRoot 'plugins'
                $info.Environment['QT_QPA_PLATFORM_PLUGIN_PATH']=Join-Path $QtRoot 'plugins/platforms'
                $info.Environment['QT_QPA_PLATFORM']='offscreen'
                $info.Environment['QT_MEDIA_BACKEND']='ffmpeg'
                $arguments=@("--$mode",'--input',$inputFile)
                if ($fixture.Channels -eq 1) { $arguments += '--mono' }
                if ($mode -eq 'worker') { $arguments += @('--cycles', [string]$Cycles) }
                if ($mode -eq 'worker' -and $device -ne 'default') { $arguments += @('--device',$device) }
                if ($CaptureDump) { $arguments += @('--dump-on-crash',$dump) }
                foreach ($argument in $arguments) { $info.ArgumentList.Add($argument) }
                $process = [Diagnostics.Process]::new(); $process.StartInfo=$info
                $watch = [Diagnostics.Stopwatch]::StartNew()
                if (!$process.Start()) { throw 'Cannot start diagnostic child' }
                $stdout=$process.StandardOutput.ReadToEndAsync(); $stderr=$process.StandardError.ReadToEndAsync()
                $timedOut=!$process.WaitForExit($TimeoutSeconds*1000)
                if ($timedOut) { $process.Kill($true); $process.WaitForExit() }
                $output=$stderr.GetAwaiter().GetResult()+$stdout.GetAwaiter().GetResult()
                $exitCode=$process.ExitCode; $watch.Stop(); $process.Dispose()
                $output | Set-Content -LiteralPath (Join-Path $ResultsDirectory ($name + '.jsonl')) -Encoding utf8
                $records=@($output -split '\r?\n' | Where-Object { $_.StartsWith('{') } | ForEach-Object { try { $_ | ConvertFrom-Json } catch {} })
                $version=$records | Where-Object qt | Select-Object -First 1
                $environmentIssues=@()
                if (!$version) { $environmentIssues+='Missing Qt inventory' }
                if ($version.qt -ne $expectedQtVersion -or $version.compiledQt -ne $expectedQtVersion -or
                    $version.compiledQtMultimedia -ne $expectedQtVersion) {
                    $environmentIssues+='Compiled and loaded Qt/Qt Multimedia must match the selected kit'
                }
                if ($expectedCompiler -and $version.compiler -ne $expectedCompiler) { $environmentIssues+='Compiler does not match CompilerBin' }
                foreach ($module in $version.loadedModules.PSObject.Properties) {
                    if (!$module.Value.StartsWith($QtRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
                        $environmentIssues+="Mixed Qt/FFmpeg module: $($module.Value)"
                    }
                }
                $pluginPath=$version.loadedModules.ffmpegmediaplugin
                $pluginVersion=if ($pluginPath) { [Diagnostics.FileVersionInfo]::GetVersionInfo($pluginPath).FileVersion } else { '' }
                if (!$pluginPath -or !$version.ffmpeg) { $environmentIssues+='FFmpeg backend inventory missing' }
                foreach ($qtModule in @('Qt6Multimedia','ffmpegmediaplugin')) {
                    $path=$version.loadedModules.$qtModule
                    if (!$path) { $environmentIssues+="Missing module: $qtModule"; continue }
                    $fileVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($path)
                    if ("$($fileVersion.FileMajorPart).$($fileVersion.FileMinorPart).$($fileVersion.FileBuildPart)" -ne $expectedQtVersion) {
                        $environmentIssues+="Module version mismatch: $qtModule"
                    }
                }
                $outcome=if ($timedOut) { 'Timeout' } elseif ($exitCode -lt 0) { 'NativeCrash' }
                    elseif ($exitCode -eq 0) { 'Success' }
                    elseif (@($records | Where-Object formatUnsupported -eq $true).Count) { 'UnsupportedFormat' }
                    else { 'Failure' }
                $lastCycle=$records | Where-Object { $_.PSObject.Properties.Name -contains 'beginCycle' } | Select-Object -Last 1
                $lastPhase=$records | Where-Object phase | Select-Object -Last 1
                $results.Add([pscustomobject]@{
                    Fixture=$fixture.Name; Mode=$mode; Device=$device; Trial=$trial; Cycles=$Cycles
                    ExitCode=$exitCode; ExitHex=$exitCode.ToString('X8'); TimedOut=$timedOut
                    LastCycle=$lastCycle.beginCycle; LastPhase=$lastPhase.phase; ElapsedMs=$watch.ElapsedMilliseconds
                    DumpBytes=$(if(Test-Path -LiteralPath $dump){(Get-Item -LiteralPath $dump).Length}else{0})
                    Log=$name+'.jsonl'; Qt=$version.qt; FFmpeg=$version.ffmpeg
                    Outcome=$outcome; Comparable=($environmentIssues.Count -eq 0); EnvironmentIssues=$environmentIssues
                    CompiledQt=$version.compiledQt; CompiledQtMultimedia=$version.compiledQtMultimedia
                    Compiler=$version.compiler; Architecture=$version.architecture; PluginVersion=$pluginVersion
                })
                $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $ResultsDirectory 'matrix.json') -Encoding utf8
                Write-Host "$name exit=$exitCode timeout=$timedOut lastCycle=$($lastCycle.beginCycle)"
            }
        }
    }
}
$results | Group-Object Fixture,Mode,Device,Outcome,Comparable | ForEach-Object {
    [pscustomobject]@{Group=$_.Name; Trials=$_.Count}
} | Format-Table
if (@($results | Where-Object {$_.ExitCode -ne 0 -or $_.TimedOut -or !$_.Comparable}).Count) { exit 1 }
