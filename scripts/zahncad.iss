#define AppVersion "0.3.1"
#ifndef PayloadDir
  #define PayloadDir "..\out\ZahnCAD-installer-payload"
#endif

[Setup]
AppId=ZahnCAD.Research
AppName=ZahnCAD
AppVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\ZahnCAD
DefaultGroupName=ZahnCAD
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\out
OutputBaseFilename=ZahnCAD-{#AppVersion}-Windows-x64-Setup
WizardStyle=modern

[Files]
Source: "{#PayloadDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\ZahnCAD"; Filename: "{app}\zahncad.exe"; WorkingDir: "{app}"
