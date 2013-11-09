; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "swSSO"
#define MyAppVersion "0.98"
#define MyAppURL "www.swsso.fr"
#define MyAppExeName "swSSO.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{8266EBBD-3ECD-4FFA-9B6B-9E44CE242E70}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=licence.txt
OutputBaseFilename=swSSO-setup-0.98
Compression=lzma
SolidCompression=yes

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1
Name: "startupicon"; Description: "{cm:CreateStartupIcon}"

[Files]
Source: "E:\swSSO\Dev\Release\swSSO.exe"; DestDir: "{app}"; Flags: ignoreversion uninsrestartdelete
Source: "E:\swSSO\Dev\Release\swSSOCM.dll"; DestDir: "{app}"; Flags: ignoreversion uninsrestartdelete; Check: not IsWin64; AfterInstall: RegisterCM('{app}\swSSOCM.dll')
Source: "E:\swSSO\Dev\Release\swSSOSVC.exe"; DestDir: "{app}"; Flags: ignoreversion uninsrestartdelete
Source: "E:\swSSO\Dev\x64\Release\swSSOCM.dll"; DestDir: "{app}"; Flags: ignoreversion uninsrestartdelete; Check: IsWin64; AfterInstall: RegisterCM('{app}\swSSOCM.dll')

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{commonstartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startupicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\swSSOSVC.exe"; Parameters: "install"

[UninstallRun]
Filename: "{app}\swSSOSVC.exe"; Parameters: "uninstall"

[Registry]
Root: "HKLM"; Subkey: "SYSTEM\CurrentControlSet\services\swSSOCM"; ValueType: none; Flags: uninsdeletekey
Root: "HKLM"; Subkey: "SYSTEM\CurrentControlSet\services\swSSOCM\NetworkProvider"; ValueType: none; Flags: uninsdeletekey
Root: "HKLM"; Subkey: "SYSTEM\CurrentControlSet\services\swSSOCM\NetworkProvider"; ValueType: string; ValueName: "AuthentProviderPath"; ValueData: "{app}\swssoCM.dll"; Flags: uninsdeletevalue
Root: "HKLM"; Subkey: "SYSTEM\CurrentControlSet\services\swSSOCM\NetworkProvider"; ValueType: dword; ValueName: "Class"; ValueData: "2"; Flags: uninsdeletevalue
Root: "HKLM"; Subkey: "SYSTEM\CurrentControlSet\services\swSSOCM\NetworkProvider"; ValueType: string; ValueName: "Name"; ValueData: "swSSOCM"; Flags: uninsdeletevalue
Root: "HKLM"; Subkey: "SYSTEM\CurrentControlSet\services\swSSOCM\NetworkProvider"; ValueType: string; ValueName: "ProviderPath"; ValueData: "{app}\swssoCM.dll"; Flags: uninsdeletevalue
Root: "HKLM32"; Subkey: "SOFTWARE\swSSO\GlobalPolicy"; ValueType: dword; ValueName: "PasswordChoiceLevel"; ValueData: "4"

[UninstallDelete]
Type: files; Name: "{userappdata}\swSSO.ini"

[CustomMessages]
french.CreateStartupIcon=Lancer swSSO au d�marrage de Windows

[Code]
procedure RegisterCM(FileName: String);
var
  strProviderOrder: String;
begin
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\NetworkProvider\Order',
     'ProviderOrder', strProviderOrder) then
  begin
    //MsgBox('strProviderOrder=' + strProviderOrder, mbInformation, MB_OK);
    // si swSSOCM d�j� pr�sent on ne fait rien, sinon on l'ajoute
    if Pos('swSSOCM',strProviderOrder)=0 then
    begin
      RegWriteStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\NetworkProvider\Order',
       'ProviderOrder', strProviderOrder+',swSSOCM');
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  FindRec: TFindRec;
begin
  if CurUninstallStep=usUninstall then begin
    // Si fichier .ini dans le dossier App, demande � l'utilisateur de confirmer avant de supprimer
    if FindFirst(ExpandConstant('{app}\*.ini'), FindRec) then begin
      if MsgBox('Si vous souhaitez conserver votre fichier de mots de passe, cliquer sur Oui. Sinon, cliquer sur Non et tout sera supprim�.', mbConfirmation, MB_YESNO) = IDNO then begin
        //DelTree(ExpandConstant('{app}'), True, True, True);
        DelTree(ExpandConstant('{app}\*.ini'), False, True, False);
      end;
    end;
  end;
end;

function NeedRestart(): Boolean;
begin
  Result := True;
end;

function UninstallNeedRestart(): Boolean;
begin
  Result := True;
end;
