#define MyAppName "PMX"
#define MyAppVersion "0.2.0-alpha.2"
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
LicenseFile=..\LICENSE
InfoAfterFile=..\NOTICE.txt
AppPublisherURL=https://github.com/KaranBarua01/PMX

[Files]
Source: "..\build\PMX_artefacts\Release\PMX.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\distribution\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\dist\BUILD-INFO.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\SOURCE.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\PMX"; Filename: "{app}\{#MyAppExeName}"
Name: "{autoprograms}\PMX Source"; Filename: "https://github.com/KaranBarua01/PMX"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch PMX"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Intentionally empty. User presets/assets/loops live under {localappdata}\PMX and are preserved.
