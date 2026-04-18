#define MyAppName "RD"
#define MyAppVersion "0.0.29"
#define MyAppPublisher "recluse-audio"
#define MyAppURL "https://recluse-audio.com"
#define VST3Source SourcePath + "\..\..\BUILD\RD_artefacts\Release\VST3\RD.vst3"
#define OutputDir SourcePath + "\BUILD"

[Setup]
AppId={{5603754C-1841-5E3D-B172-5AD62C693DDD}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}
UninstallFilesDir={app}
DefaultGroupName={#MyAppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename={#MyAppName}_v{#MyAppVersion}_Windows_Installer
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#VST3Source}\*"; DestDir: "{commoncf}\VST3\{#MyAppName}.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
