$ErrorActionPreference = 'Stop'
$installer = (Resolve-Path 'dist/PMX-0.2.0-alpha.2-x64.exe').Path
$installFolder = Join-Path $env:RUNNER_TEMP 'PMX-installer-test'
$userData = Join-Path $env:LOCALAPPDATA 'PMX'
$sentinel = Join-Path $userData 'installer-preservation-test.txt'
New-Item -ItemType Directory -Path $userData -Force | Out-Null
'PMX user content must survive install and uninstall.' | Set-Content -LiteralPath $sentinel
$arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',('/DIR="' + $installFolder + '"'))
foreach ($attempt in 1..2) {
    $setup = Start-Process -FilePath $installer -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
    if ($setup.ExitCode -ne 0) { throw "Installer exited with $($setup.ExitCode)." }
    if (!(Test-Path -LiteralPath (Join-Path $installFolder 'PMX.exe'))) { throw 'Installed EXE missing.' }
    if ((Get-Content -LiteralPath $sentinel -Raw).Trim() -ne 'PMX user content must survive install and uninstall.') { throw 'Installer changed user data.' }
}
$app = Start-Process -FilePath (Join-Path $installFolder 'PMX.exe') -WindowStyle Hidden -PassThru
Start-Sleep -Seconds 5
$app.Refresh()
if ($app.HasExited) { throw "PMX exited during startup: $($app.ExitCode)." }
if ($app.MainWindowTitle -ne 'PMX') { throw 'PMX did not create its native main window.' }
if (!$app.CloseMainWindow()) { Stop-Process -Id $app.Id }
elseif (!$app.WaitForExit(10000)) { Stop-Process -Id $app.Id }
$uninstall = Start-Process -FilePath (Join-Path $installFolder 'unins000.exe') -ArgumentList '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART' -WindowStyle Hidden -PassThru -Wait
if ($uninstall.ExitCode -ne 0) { throw "Uninstaller exited with $($uninstall.ExitCode)." }
if (!(Test-Path -LiteralPath $sentinel)) { throw 'Uninstaller removed user content.' }
Write-Output 'Installer, repair install, native startup and user-data preservation passed. No hardware audio was tested.'

