#define MyAppName "PMX"
#define MyAppVersion "0.2.0-alpha.1"
#define MyAppExeName "PMX.exe"

[Setup]
AppId={{D2923B6B-6D2D-4AB3-B2F1-77F6468D0B0E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=PMX
DefaultDirName={localappdata}\Programs\PMX
DefaultGroupName=PMX
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=PMX-{#MyAppVersion}-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}

[Files]
Source: "..\build\PMX_artefacts\Release\PMX.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\PMX"; Filename: "{app}\{#MyAppExeName}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch PMX"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Intentionally empty. User presets/assets/loops live under {localappdata}\PMX and are preserved.
