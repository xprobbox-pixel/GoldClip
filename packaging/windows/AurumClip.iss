#ifndef AppVersion
 #error AppVersion must be supplied by package-windows.ps1
#endif
[Setup]
AppId={{D2E2D57F-C188-4C95-AF83-5B5F5B0F8AF1}
AppName=AURUM CLIP
AppVersion={#AppVersion}
AppPublisher={#CompanyName}
DefaultDirName={autopf}\Aurum Clip
DefaultGroupName=Aurum Clip
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
OutputDir={#OutputDir}
OutputBaseFilename=AURUM-CLIP-{#AppVersion}-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
UninstallDisplayName=AURUM CLIP
WizardStyle=modern
LicenseFile=..\DEVELOPMENT-NOTICE.txt
[Types]
Name: full; Description: "Master and Track - VST3 and standalone"
Name: custom; Description: "Custom installation"; Flags: iscustom
[Components]
Name: master; Description: "Master edition"; Types: full custom
Name: master\vst3; Description: "Master VST3"; Types: full custom
Name: master\app; Description: "Master standalone"; Types: full custom
Name: track; Description: "Track edition"; Types: full custom
Name: track\vst3; Description: "Track VST3"; Types: full custom
Name: track\app; Description: "Track standalone"; Types: full custom
[Files]
Source: "{#StageDir}\Master\VST3\AURUM CLIP Master.vst3\*"; DestDir: "{commoncf64}\VST3\AURUM CLIP Master.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: master\vst3
Source: "{#StageDir}\Track\VST3\AURUM CLIP Track.vst3\*"; DestDir: "{commoncf64}\VST3\AURUM CLIP Track.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: track\vst3
Source: "{#StageDir}\Master\Standalone\AURUM CLIP Master.exe"; DestDir: "{app}\Master"; Flags: ignoreversion; Components: master\app
Source: "{#StageDir}\Track\Standalone\AURUM CLIP Track.exe"; DestDir: "{app}\Track"; Flags: ignoreversion; Components: track\app
[Icons]
Name: "{group}\AURUM CLIP Master"; Filename: "{app}\Master\AURUM CLIP Master.exe"; Components: master\app
Name: "{group}\AURUM CLIP Track"; Filename: "{app}\Track\AURUM CLIP Track.exe"; Components: track\app
Name: "{group}\Uninstall AURUM CLIP"; Filename: "{uninstallexe}"
