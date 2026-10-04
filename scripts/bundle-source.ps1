$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildRoot = Join-Path $repoRoot 'build'
$staging = Join-Path $buildRoot 'source-bundle'
New-Item -ItemType Directory -Path $staging -Force | Out-Null
$appZip = Join-Path $buildRoot 'app-source.zip'
git -C $repoRoot archive --format=zip --output=$appZip HEAD
if ($LASTEXITCODE -ne 0) { throw 'Application source archive failed.' }
Expand-Archive -LiteralPath $appZip -DestinationPath $staging -Force
$deps = Join-Path $staging 'deps'
New-Item -ItemType Directory -Path $deps -Force | Out-Null
$checkouts = @{JUCE=(Join-Path $buildRoot '_deps/juce-src'); NAMCore=(Join-Path $buildRoot '_deps/namcore-src')}
foreach ($name in $checkouts.Keys) {
    $checkout = $checkouts[$name]
    $dependencyZip = Join-Path $buildRoot "$name-source.zip"
    git -C $checkout archive --format=zip --output=$dependencyZip HEAD
    if ($LASTEXITCODE -ne 0) { throw "$name source archive failed." }
    Expand-Archive -LiteralPath $dependencyZip -DestinationPath (Join-Path $deps $name) -Force
}
foreach ($relative in @('Dependencies/eigen','Dependencies/AudioDSPTools','Dependencies/AudioDSPTools/Dependencies/eigen')) {
    $checkout = Join-Path $checkouts.NAMCore $relative
    $dependencyZip = Join-Path $buildRoot ($relative.Replace('/','-') + '-source.zip')
    git -C $checkout archive --format=zip --output=$dependencyZip HEAD
    if ($LASTEXITCODE -ne 0) { throw "$relative source archive failed." }
    Expand-Archive -LiteralPath $dependencyZip -DestinationPath (Join-Path (Join-Path $deps 'NAMCore') $relative) -Force
}
$revision = git -C $repoRoot rev-parse HEAD
$info = @"
PMX source commit: $revision
JUCE: be29c81492b6151c8ea8d14c840e1311963b3a83
NAMCore: 0b3d3c97b0859a3a8c92a8628c4dd89a25eb5842
Windows x64 MSVC Release; ASIO; AGPLv3
"@
$info | Set-Content -LiteralPath (Join-Path $staging 'BUILD-INFO.txt') -Encoding utf8
New-Item -ItemType Directory -Path (Join-Path $repoRoot 'dist') -Force | Out-Null
$info | Set-Content -LiteralPath (Join-Path $repoRoot 'dist/BUILD-INFO.txt') -Encoding utf8
Compress-Archive -Path (Join-Path $staging '*') -DestinationPath (Join-Path $repoRoot 'dist/PMX-source.zip') -Force

