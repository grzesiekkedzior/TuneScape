# Requires PowerShell 7 and a separately supplied FFmpeg/ffprobe CLI. No downloads or installation.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$FFmpeg,
    [Parameter(Mandatory)][string]$FFprobe,
    [Parameter(Mandatory)][string]$DiagnosticsExecutable,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if ((Test-Path -LiteralPath $OutputDirectory) -and
    (!(Test-Path -LiteralPath $OutputDirectory -PathType Container) -or
     (Get-ChildItem -LiteralPath $OutputDirectory -Force | Select-Object -First 1))) {
    throw 'Use a fresh, empty fixture directory; existing files are preserved'
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$version = & $FFmpeg -version
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify FFmpeg CLI' }
$version | Set-Content -LiteralPath (Join-Path $OutputDirectory 'encoder-version.txt') -Encoding utf8
$specifications = @(
    @{ Name='synthetic-mono-mp2'; Extension='mp2'; Rate=44100; Channels=1; Codec='mp2'; Synthetic=$true },
    @{ Name='synthetic-stereo-mp3'; Extension='mp3'; Rate=44100; Channels=2; Codec='mp3'; Synthetic=$true },
    @{ Name='encoded-mono-mp2-44100'; Extension='mp2'; Rate=44100; Channels=1; Codec='mp2' },
    @{ Name='encoded-mono-mp2-48000'; Extension='mp2'; Rate=48000; Channels=1; Codec='mp2' },
    @{ Name='encoded-stereo-mp2'; Extension='mp2'; Rate=44100; Channels=2; Codec='mp2' },
    @{ Name='encoded-mono-mp3'; Extension='mp3'; Rate=44100; Channels=1; Codec='mp3' },
    @{ Name='encoded-stereo-mp3'; Extension='mp3'; Rate=44100; Channels=2; Codec='mp3' },
    @{ Name='encoded-mono-pcm'; Extension='wav'; Rate=44100; Channels=1; Codec='pcm_s16le' }
)
$manifest = foreach ($spec in $specifications) {
    $path = Join-Path $OutputDirectory ($spec.Name + '.' + $spec.Extension)
    if ($spec.Synthetic) {
        $recipe = @('--write-fixture', $path)
        if ($spec.Channels -eq 1) { $recipe += '--mono' }
        & $DiagnosticsExecutable @recipe
    } else {
        $codec = if ($spec.Codec -eq 'mp3') { 'libmp3lame' } else { $spec.Codec }
        $recipe = @('-hide_banner','-v','error','-n','-f','lavfi','-i',
                    "sine=frequency=997:sample_rate=$($spec.Rate):duration=6",
                    '-ac', [string]$spec.Channels, '-c:a', $codec)
        if ($spec.Codec -ne 'pcm_s16le') { $recipe += @('-b:a', '128k') }
        if ($spec.Codec -eq 'mp3') { $recipe += @('-write_xing','0','-id3v2_version','0','-write_id3v1','0') }
        $recipe += $path
        & $FFmpeg @recipe
    }
    if ($LASTEXITCODE -ne 0) { throw "Cannot create $($spec.Name)" }
    $probe = & $FFprobe -v error -select_streams a:0 -show_streams -show_format -of json $path
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $path" }
    $probe | Set-Content -LiteralPath (Join-Path $OutputDirectory ($spec.Name + '.probe.json')) -Encoding utf8
    $stream = ($probe | ConvertFrom-Json).streams[0]
    if ($stream.codec_name -ne $spec.Codec -or [int]$stream.sample_rate -ne $spec.Rate -or $stream.channels -ne $spec.Channels) {
        throw "Unexpected codec parameters for $($spec.Name)"
    }
    $validation = & $FFmpeg -hide_banner -v error -xerror -i $path -f null - 2>&1
    $decodeExit = $LASTEXITCODE
    $validation | Set-Content -LiteralPath (Join-Path $OutputDirectory ($spec.Name + '.validation.txt')) -Encoding utf8
    if ($decodeExit -ne 0) { throw "Independent decoding failed for $($spec.Name)" }
    [pscustomobject]@{
        Name=$spec.Name; File=[IO.Path]::GetFileName($path); Codec=$stream.codec_name
        Channels=$stream.channels; Rate=[int]$stream.sample_rate; SampleFormat=$stream.sample_fmt
        BitRate=$stream.bit_rate; Bytes=(Get-Item -LiteralPath $path).Length
        Sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        Synthetic=[bool]$spec.Synthetic; ValidationExit=$decodeExit; Recipe=$recipe
    }
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'fixtures.json') -Encoding utf8
$manifest | Select-Object Name,Codec,Channels,Rate,SampleFormat,ValidationExit | Format-Table
