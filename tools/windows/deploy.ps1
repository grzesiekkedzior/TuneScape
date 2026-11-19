# PowerShell 7; application-only staging, never an installer.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$QtRoot,
    [Parameter(Mandatory)][string]$CompilerBin,
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$QtRoot=(Resolve-Path -LiteralPath $QtRoot).Path
$CompilerBin=(Resolve-Path -LiteralPath $CompilerBin).Path
$Executable=(Resolve-Path -LiteralPath $Executable).Path
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
if((Test-Path -LiteralPath $OutputDirectory) -and (Get-ChildItem -LiteralPath $OutputDirectory -Force)) {
    throw 'Deployment requires an empty directory; existing files are preserved'
}
if ((& "$QtRoot/bin/qmake.exe" -query QT_VERSION) -ne '6.12.0') { throw 'Expected Qt 6.12.0' }
if ((& "$CompilerBin/g++.exe" -dumpfullversion) -ne '15.1.0') { throw 'Expected GCC 15.1.0' }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$env:PATH="$QtRoot/bin;$CompilerBin;$env:SystemRoot/System32;$env:SystemRoot"
Copy-Item -LiteralPath $Executable -Destination $OutputDirectory
$target=Join-Path $OutputDirectory ([IO.Path]::GetFileName($Executable))
& "$QtRoot/bin/windeployqt.exe" --release --compiler-runtime --no-translations --no-patchqt --dir $OutputDirectory $target
if($LASTEXITCODE){throw 'windeployqt failed'}
"[Paths]`nPrefix=.`nPlugins=." | Set-Content -LiteralPath (Join-Path $OutputDirectory 'qt.conf') -Encoding ascii
# Explicit fallback for FFmpeg runtime dependencies, always from the selected SDK.
Get-ChildItem -LiteralPath "$QtRoot/bin" -File | Where-Object Name -match '^(avcodec|avformat|avutil|swresample|swscale)-\d+\.dll$' | ForEach-Object {
    $path=Join-Path $OutputDirectory $_.Name
    if(!(Test-Path -LiteralPath $path)){Copy-Item -LiteralPath $_.FullName -Destination $path}
}
foreach($required in @('Qt6Core.dll','Qt6Gui.dll','Qt6Widgets.dll','Qt6Network.dll','Qt6Multimedia.dll','Qt6Svg.dll',
                       'platforms/qwindows.dll','multimedia/ffmpegmediaplugin.dll',
                       'avcodec-63.dll','avformat-63.dll','avutil-61.dll','swresample-7.dll',
                       'libstdc++-6.dll','libgcc_s_seh-1.dll','libwinpthread-1.dll')) {
    if(!(Test-Path -LiteralPath (Join-Path $OutputDirectory $required))){throw "Missing deployment file: $required"}
}
$manifest=foreach($file in Get-ChildItem -LiteralPath $OutputDirectory -Recurse -File){
    $relative=[IO.Path]::GetRelativePath($OutputDirectory,$file.FullName)
    if($file.Name -match '^Qt6.*\.dll$' -and $file.VersionInfo.FileVersion -ne '6.12.0.0'){throw "Wrong Qt DLL: $relative"}
    if($file.Name -match '^(avcodec-60|avformat-60|avutil-58|swresample-4|swscale-7)\.dll$'){throw "Old FFmpeg: $relative"}
    if($file.Extension -eq '.dll'){
        $candidates=@((Join-Path "$QtRoot/bin" $file.Name),(Join-Path $CompilerBin $file.Name),(Join-Path "$QtRoot/plugins" $relative))
        $hash=(Get-FileHash -LiteralPath $file.FullName).Hash
        $matching=@($candidates | Where-Object {(Test-Path -LiteralPath $_) -and (Get-FileHash -LiteralPath $_).Hash -eq $hash})
        if(!$matching.Count){throw "Unverified DLL provenance: $relative"}
    }
    [pscustomobject]@{File=$relative;Bytes=$file.Length;Version=$file.VersionInfo.FileVersion;Sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'deployment-manifest.json') -Encoding utf8
