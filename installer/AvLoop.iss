; AvLoop's Windows installer (spec §1.2 item 14, §3.1, §3.8; D-014): Inno Setup 6 over deploy\ (scripts\deploy.ps1).
; Per-user by default, per-machine by the user's choice; the Start menu entry and an optional desktop icon; the .avloop
; association; the licences shown and installed. Uninstalling removes the program, the association, the autosave, the
; logs and the disk cache, and keeps the settings, presets and batch queue (%APPDATA%\AvLoop). Not signed (§1.3).
;   ISCC installer\AvLoop.iss          -> installer\Output\AvLoop-<version>-setup.exe
; The version is avloop.exe's own (from the root CMakeLists.txt); /DAppVersion=x.y.z overrides it.

#define SourceDir AddBackslash(SourcePath) + "..\deploy"
#ifndef AppVersion
  #define AppVersion GetStringFileInfo(SourceDir + "\avloop.exe", "ProductVersion")
#endif
#if AppVersion == ""
  #error deploy\avloop.exe has no version: run scripts\deploy.ps1 first
#endif

[Setup]
AppId={{61608584-D166-4BF0-859A-7E3EB4E9CB7A}
AppName=AvLoop
AppVersion={#AppVersion}
AppVerName=AvLoop {#AppVersion}
AppPublisher=AvLoop
AppPublisherURL=https://github.com/rubeldbc/avLoop
AppSupportURL=https://github.com/rubeldbc/avLoop/issues
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\AvLoop
DefaultGroupName=AvLoop
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
LicenseFile=..\LICENSE
InfoBeforeFile=..\LICENSE-THIRD-PARTY.md
SetupIconFile=..\resources\app\avloop.ico
UninstallDisplayIcon={app}\avloop.exe
UninstallDisplayName=AvLoop
OutputDir=Output
OutputBaseFilename=AvLoop-{#AppVersion}-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ChangesAssociations=yes
CloseApplications=yes

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\AvLoop"; Filename: "{app}\avloop.exe"
Name: "{autodesktop}\AvLoop"; Filename: "{app}\avloop.exe"; Tasks: desktopicon

[Registry]
; .avloop → AvLoop.Project, opened by "avloop.exe" "%1" with the app's icon (HKA: the user's or the machine's classes).
Root: HKA; Subkey: "Software\Classes\.avloop"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.avloop"; ValueType: string; ValueName: ""; ValueData: "AvLoop.Project"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.avloop\OpenWithProgids"; ValueType: none; ValueName: "AvLoop.Project"; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\AvLoop.Project"; ValueType: string; ValueName: ""; ValueData: "AvLoop project"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\AvLoop.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\avloop.exe"",0"
Root: HKA; Subkey: "Software\Classes\AvLoop.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\avloop.exe"" ""%1"""

[Run]
Filename: "{app}\avloop.exe"; Description: "{cm:LaunchProgram,AvLoop}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; What AvLoop made for itself: the autosave and the logs (%LOCALAPPDATA%\AvLoop), the disk cache
; (%LOCALAPPDATA%\cache\AvLoop). The settings, presets and queue (%APPDATA%\AvLoop) stay.
Type: filesandordirs; Name: "{localappdata}\AvLoop"
Type: filesandordirs; Name: "{localappdata}\cache\AvLoop"
