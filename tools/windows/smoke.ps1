# Launch only our staged application; WM_CLOSE requests normal Qt shutdown.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Distribution,[Parameter(Mandatory)][string]$ResultsDirectory)
$ErrorActionPreference='Stop'
$Distribution=(Resolve-Path -LiteralPath $Distribution).Path
if((Test-Path -LiteralPath $ResultsDirectory) -and (Get-ChildItem -LiteralPath $ResultsDirectory -Force)) {
    throw 'Smoke results require an empty directory; earlier logs are preserved'
}
New-Item -ItemType Directory -Path $ResultsDirectory -Force | Out-Null
$ResultsDirectory=(Resolve-Path -LiteralPath $ResultsDirectory).Path
$env:PATH="$env:SystemRoot/System32;$env:SystemRoot"
Get-ChildItem Env: | Where-Object Name -like 'QT_*' | ForEach-Object {Remove-Item -LiteralPath ('Env:'+ $_.Name)}
$env:QT_DEBUG_PLUGINS='1'
$env:QT_FORCE_STDERR_LOGGING='1'
$p=Start-Process -FilePath (Join-Path $Distribution 'TuneScapeCore.exe') -WorkingDirectory $ResultsDirectory -WindowStyle Normal -PassThru -RedirectStandardError (Join-Path $ResultsDirectory 'stderr.txt') -RedirectStandardOutput (Join-Path $ResultsDirectory 'stdout.txt')
try {
    $deadline=(Get-Date).AddSeconds(20)
    do {Start-Sleep -Milliseconds 250; $p.Refresh()} until($p.HasExited -or $p.MainWindowHandle -ne 0 -or (Get-Date) -gt $deadline)
    if($p.HasExited -or $p.MainWindowHandle -eq 0){throw 'No main window created'}
    $title=$p.MainWindowTitle
    if($title -ne 'TuneScape'){throw "Unexpected main window: $title"}
    $modules=@($p.Modules | Where-Object {$_.ModuleName -match '^(Qt6|qwindows|ffmpeg|avcodec|avformat|avutil|swresample|swscale|libstdc\+\+|libgcc|libwinpthread)' } | ForEach-Object {$_.FileName})
    foreach($module in $modules){if(!$module.StartsWith($Distribution+'\',[StringComparison]::OrdinalIgnoreCase)){throw "SDK dependency loaded: $module"}}
    if(!@($modules | Where-Object {$_ -like '*\platforms\qwindows.dll'}).Count){throw 'Packaged qwindows was not loaded'}
    $modules | Set-Content -LiteralPath (Join-Path $ResultsDirectory 'loaded-modules.txt')
    if(!$p.CloseMainWindow()){throw 'WM_CLOSE request failed'}
    if(!$p.WaitForExit(15000)){throw 'Application did not close normally'}
    if($p.ExitCode -ne 0){throw "Application exit: $($p.ExitCode)"}
    [pscustomobject]@{Window=$title;ExitCode=$p.ExitCode;Path=$env:PATH;Modules=$modules} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ResultsDirectory 'smoke.json') -Encoding utf8
} finally {if(!$p.HasExited){$p.Kill()}; $p.Dispose()}
