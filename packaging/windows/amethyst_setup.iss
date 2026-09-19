; =====================================================================
; Amethyst IDE — Inno Setup Script
; Developer: Atimenka
; Repository: https://github.com/Atimenka/Amethyst
; =====================================================================

#define MyAppName "Amethyst"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Atimenka"
#define MyAppURL "https://github.com/Atimenka/Amethyst"
#define MyAppExeName "amethyst.exe"

[Setup]
; Basic Application Info
AppId={{C789231A-684D-4E92-B7AC-F3E85C77A912}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases

; Installation Directories
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; Visuals & Branding
SetupIconFile=..\..\src\resources\icons\amethyst.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
WizardStyle=modern
WizardSizePercent=105

; Output Configuration
OutputDir=..\..\dist
OutputBaseFilename=Amethyst_Setup_x64
Compression=lzma2/ultra64
SolidCompression=yes

; Architecture (64-bit Windows)
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Privileges: allows installing for current user without admin, or all users with admin
PrivilegesRequiredOverridesAllowed=dialog commandline

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "contextmenu"; Description: "Добавить «Открыть в Amethyst» в контекстное меню Проводника"; GroupDescription: "Интеграция с системой:"; Flags: unchecked

[Files]
; Main Executable & deployed Qt runtime DLLs from the staging directory
Source: "bundle\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\src\resources\icons\amethyst.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\amethyst.ico"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\amethyst.ico"; Tasks: desktopicon

[Registry]
; Context Menu: Directory Background
Root: HKA; Subkey: "Software\Classes\Directory\Background\shell\Amethyst"; ValueType: string; ValueName: ""; ValueData: "Открыть с помощью Amethyst"; Tasks: contextmenu; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Directory\Background\shell\Amethyst"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKA; Subkey: "Software\Classes\Directory\Background\shell\Amethyst\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%V"""; Tasks: contextmenu

; Context Menu: Directory Folder
Root: HKA; Subkey: "Software\Classes\Directory\shell\Amethyst"; ValueType: string; ValueName: ""; ValueData: "Открыть папку в Amethyst"; Tasks: contextmenu; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Directory\shell\Amethyst"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKA; Subkey: "Software\Classes\Directory\shell\Amethyst\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
