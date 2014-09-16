;
;    This file is part of SourceOP.
;
;    SourceOP is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.
;
;    SourceOP is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
;

; -- install script.iss --
; Installation file for SourceOP.
; Built with Inno Setup Compiler 5.3.9

[Setup]
AppName=SourceOP
AppVerName=SourceOP Beta Version 0.9.17.543
AppPublisher=SourceOP.com
AppPublisherURL=http://www.sourceop.com/
AppCopyright=Copyright (C) 2005-2013 SourceOP.com
DefaultDirName={code:GetMyInstallPath}
DefaultGroupName=SourceOP
UninstallFilesDir={code:GetFinalInstallPath}\addons\SourceOP\uninstall
;UninstallDisplayIcon={app}\SourceOPAdmin
Compression=lzma2
SolidCompression=yes
DirExistsWarning=no
AppendDefaultDirName=no
VersionInfoCopyright=Copyright (C) 2005-2013 SourceOP.com
VersionInfoVersion=0.9.17.543

[Files]
Source: "files\SourceOP\bin\*"; DestDir: "{code:GetFinalInstallPath}\..\bin"; Excludes: ".svn"; Components: main
Source: "files\SourceOP\gamemod\*"; DestDir: "{code:GetFinalInstallPath}"; Flags: recursesubdirs; Excludes: ".svn"; Components: main
Source: "..\addons\SourceOP\*"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP"; Excludes: "DF_admins.txt,DF_radio.txt,DF_radio_empty.txt,remote.crt,remote.key,.svn"; Components: main
Source: "..\addons\SourceOP\features_alloff\*"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Excludes: ".svn"; Components: main
Source: "..\addons\SourceOP\DF_admins.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP"; Flags: confirmoverwrite uninsneveruninstall; Components: main
Source: "..\addons\SourceOP\DF_radio_empty.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP"; DestName: "DF_radio.txt"; Components: main

;Feature: AdminCommands
Source: "..\addons\SourceOP\features\admincommands.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\admincommands

;Feature: AdminSayCommands
Source: "..\addons\SourceOP\features\adminsaycommands.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\adminsaycommands

;Feature: Credits
Source: "..\addons\SourceOP\features\credits.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\credits

;Feature: CvarVote
Source: "..\addons\SourceOP\features\cvarvote.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\cvarvote

;Feature: EntCommands
Source: "..\addons\SourceOP\features\entcommands.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\entcommands

;Feature: Hook
Source: "..\addons\SourceOP\features\hook.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\hook

;Feature: Jetpack
Source: "..\addons\SourceOP\features\jetpack.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\jetpack

;Feature: Killsounds
Source: "..\addons\SourceOP\features\killsounds.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\killsounds
Source: "files\KillSounds\gamemod\*"; DestDir: "{code:GetFinalInstallPath}\custom\SourceOP"; Flags: recursesubdirs; Excludes: ".svn"; Components: features\killsounds

;Feature: Lua
Source: "..\addons\SourceOP\features\lua.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\lua
Source: "..\addons\SourceOP\lua\*"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\lua"; Flags: recursesubdirs; Excludes: ".svn,PHResourcePack,PHResourcePack.zip,PHDataPack.zip,wormhole_black,*_soppriv*"; Components: features\lua

;Feature: Mapvote
Source: "..\addons\SourceOP\features\mapvote.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\mapvote

;Feature: PlayerSayCommands
Source: "..\addons\SourceOP\features\playersaycommands.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\playersaycommands

;Feature: Radio
Source: "..\addons\SourceOP\features\radio.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\radio
Source: "..\addons\SourceOP\DF_radio.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP"; Components: features\radio
Source: "files\Radio\gamemod\*"; DestDir: "{code:GetFinalInstallPath}\custom\SourceOP"; Flags: recursesubdirs; Excludes: ".svn"; Components: features\radio

;Feature: Remote
Source: "..\addons\SourceOP\features\remote.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\remote
Source: "..\addons\SourceOP\remote.crt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP"; Components: features\remote
Source: "..\addons\SourceOP\remote.key"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP"; Components: features\remote
Source: "..\addons\SourceOP\users\files\*"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\users\files"; Flags: recursesubdirs; Excludes: ".svn"; Components: features\remote
Source: "..\addons\SourceOP\users\example\*"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\users\example"; Flags: recursesubdirs; Excludes: ".svn"; Components: features\remote

;Feature: Snark
Source: "..\addons\SourceOP\features\snark.txt"; DestDir: "{code:GetFinalInstallPath}\addons\SourceOP\features"; Components: features\snark
Source: "files\Snark\gamemod\*"; DestDir: "{code:GetFinalInstallPath}\custom\SourceOP"; Flags: recursesubdirs; Excludes: ".svn"; Components: features\snark

Source: "SourceOP.url"; DestDir: "{code:GetFinalInstallPath}"
Source: "DetectDll.dll"; Flags: dontcopy

;Source: "MyProg.hlp"; DestDir: "{app}"
;Source: "Readme.txt"; DestDir: "{app}\addons\SourceOP"; Flags: isreadme

; Shouldn't do this because admins file doesn't get deleted so there is no need to
; Doesn't work anyways...
;[UninstallDelete]
;Type: files; Name: "{app}\addons\SourceOP\admintut_lock.txt"

[Components]
Name: "main"; Description: "Main Files"; Types: full compact entmod hookmod radiomod custom
Name: "features"; Description: "Features"; Types: full
Name: "features\admincommands"; Description: "Admin Commands"; Types: full compact custom
Name: "features\adminsaycommands"; Description: "Admin Chat Commands"; Types: full compact custom
Name: "features\credits"; Description: "Credits"; Types: full compact custom
Name: "features\cvarvote"; Description: "CVAR Voting (Jetpack vote)"; Types: full compact custom
Name: "features\entcommands"; Description: "Entity Commands"; Types: full compact entmod custom
Name: "features\hook"; Description: "Hook"; Types: full compact hookmod custom
Name: "features\jetpack"; Description: "Jetpack"; Types: full compact custom
Name: "features\killsounds"; Description: "Kill Sounds (Quake Sounds)"; Types: full custom
Name: "features\lua"; Description: "Lua"; Types: full
Name: "features\mapvote"; Description: "Map Voting"; Types: full compact custom
Name: "features\playersaycommands"; Description: "Player Chat Commands"; Types: full compact custom
Name: "features\radio"; Description: "Radio"; Types: full radiomod
Name: "features\remote"; Description: "Remote Connection"; Types: full compact custom
Name: "features\snark"; Description: "Snarks"; Types: full

[Types]
Name: "full"; Description: "Full installation"
Name: "entmod"; Description: "Ent Mod Only"
Name: "hookmod"; Description: "Hook Mod Only"
Name: "radiomod"; Description: "Radio Mod Only"
Name: "compact"; Description: "Compact installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Icons]
Name: "{group}\Uninstall SourceOP"; Filename: "{uninstallexe}"
Name: "{group}\SourceOP Web Site"; Filename: "{code:GetFinalInstallPath}\SourceOP.url"

[Run]
;Filename: "{code:GetFinalInstallPath}\SourceOP.url"; Description: "Go to the SourceOP web site"; Flags: postinstall shellexec skipifsilent
Filename: "http://www.sourceop.com/wiki/"; Description: "Get help for SourceOP on the SourceOP wiki"; Flags: postinstall shellexec skipifsilent

[Code]
var
  ModePage: TInputOptionWizardPage;
  ModDirPage: TInputOptionWizardPage;
  InstallDirPage: TInputDirWizardPage;
  lpSteamDir: AnsiString;
  lpUserDir: AnsiString;
  foundDir: Integer;
  foundUserDir: Integer;
  modPath: AnsiString;
  
//importing a custom DLL function
function GetSteamPath(lpSteamPath: PAnsiChar): Integer;
external 'GetSteamPath@files:DetectDll.dll stdcall';
function GetSteamUserPath(lpSteamPath: PAnsiChar): Integer;
external 'GetSteamUserPath@files:DetectDll.dll stdcall';

function GetMyInstallPath(Param: String): String;
begin
  SetLength(lpSteamDir, 255);
  SetLength(lpUserDir, 255);
  foundDir := GetSteamPath(PAnsiChar(lpSteamDir));
  foundUserDir := GetSteamUserPath(PAnsiChar(lpUserDir));
  //foundDir := 0 // Testing
  if(foundDir = 0) then
  begin;
    Result := ExpandConstant('{pf}\Steam\SteamApps');
    //MsgBox('Found dir 0', mbInformation, MB_OK);
  end else begin
    SetLength(lpSteamDir, foundDir);
    SetLength(lpUserDir, foundUserDir);
    Result := string(lpSteamDir) + '\SteamApps';
    //MsgBox(lpSteamDir, mbInformation, MB_OK);
  end;
end;

function GetMyAppPath(Param: String): String;
var
  myResult: String;
begin
  myResult := lpUserDir + '\' + modPath
  if ModePage.SelectedValueIndex = 0 then Result := myResult else Result := WizardDirValue;
end;

function GetFinalInstallPath(Param: String): String;
begin
  if ModePage.SelectedValueIndex = 0 then Result := InstallDirPage.Values[0] else Result := WizardDirValue;
end;

procedure DoPreInstall();
var
  ResultCode: Integer;

begin
  //Log('Inside DoPreInstall');
  //Exec(ExpandConstant('{tmp}\vcredist_x86.exe'), '', '', SW_SHOW, ewWaitUntilTerminated, ResultCode);
end;

procedure InitializeWizard;
begin
  ModePage := CreateInputOptionPage(wpWelcome, 'Install Mode', 'Select the installation mode.',
    'Select the installation mode to use. If you will be installing to somewhere other than the Steam folder, please select advanced.', True, False);
  ModePage.Add('Normal mode (installation path automatically detected)')
  ModePage.Add('Advanced mode (you pick the install path)')
  ModePage.Values[0] := True;
  
  ModDirPage := CreateInputOptionPage(ModePage.ID, 'What Mod?', 'Select the mod to install to.',
    'Select the mod you wish to install SourceOP to. Remember, SourceOP is a plugin not an entirely different mod. SourceOP will attach itself to the mod you pick here when starting a server. If your choice is not available, please go back and select advanced on the mode page.', True, False);
  ModDirPage.Add('Half-Life 2 Deathmatch')
  ModDirPage.Add('Team Fortress 2')
  ModDirPage.Add('Counter-Strike: Source')
  ModDirPage.Add('Day of Defeat: Source')
  ModDirPage.Add('Dedicated Server: Half-Life 2 Deathmatch')
  ModDirPage.Add('Dedicated Server: Team Fortress 2')
  ModDirPage.Add('Dedicated Server: Counter-Strike: Source')
  ModDirPage.Add('Dedicated Server: Day of Defeat: Source')
  ModDirPage.Values[5] := True;
  
  InstallDirPage := CreateInputDirPage(ModDirPage.ID,
    'Detected Install Directory', 'Is this where SourceOP should be installed?',
    'The installation has detected this installation path. Please confirm that this is accurate, then click Next. This path should have the following format: <SteamDir>\SteamApps\<UserName>\<GameDir>\<ModDir>',
    False, '');
  InstallDirPage.Add('');
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  case PageID of
  wpSelectDir:
    if ModePage.SelectedValueIndex = 0 then Result := True else Result := False;
  wpSelectProgramGroup:
    Result := True;
  ModDirPage.ID:
    if ModePage.SelectedValueIndex = 1 then result := True else Result := False;
  InstallDirPage.ID:
    if ModePage.SelectedValueIndex = 1 then result := True else Result := False;
  //wpReady:
    //if ModePage.SelectedValueIndex = 0 then Result := True else Result := False;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  case CurPageID of
    wpSelectDir:
    begin;

      MsgBox('Imporant! Do not just click next! You must select the game''s MOD folder.' #13#13 'An example for hl2mp is:' #13 'C:\Program Files\Steam\SteamApps\<UserName>\half-life 2 deathmatch\hl2mp' #13 'An example for CSS is:' #13 'C:\Program Files\Steam\SteamApps\<UserName>\counter-strike source\cstrike', mbInformation, MB_OK);
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
  begin
    DoPreInstall();
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
//var
//  ResultCode: Integer;
begin
  case CurPageID of
    wpSelectDir:
      if CompareText(WizardDirValue, GetMyInstallPath('')) = 0 then
      begin;
        MsgBox('You must select the game''s mod folder.', mbInformation, MB_OK);
        Result := False;
      end else begin
        Result := True;
      end;
    ModePage.ID:
      if ((foundDir = 0) or (foundUserDir = 0)) and (ModePage.SelectedValueIndex = 0) then
      begin;
        MsgBox('Sorry, your Steam installation path was not detected. Advanced mode is required.', mbInformation, MB_OK);
        Result := False;
      end else begin
        Result := True;
      end;
    ModDirPage.ID:
      begin;
      if ModDirPage.SelectedValueIndex = 0 then
        modPath := 'common\Half-Life 2 Deathmatch\hl2mp'
      else if ModDirPage.SelectedValueIndex = 1 then
        modPath := 'team fortress 2\tf'
      else if ModDirPage.SelectedValueIndex = 2 then
        modPath := 'counter-strike source\cstrike'
      else if ModDirPage.SelectedValueIndex = 3 then
        modPath := 'day of defeat source\dod'
      else if ModDirPage.SelectedValueIndex = 4 then
        modPath := 'source 2007 dedicated server\hl2mp'
      else if ModDirPage.SelectedValueIndex = 5 then
        modPath := 'source 2007 dedicated server\tf'
      else if ModDirPage.SelectedValueIndex = 6 then
        modPath := 'source 2007 dedicated server\cstrike'
      else if ModDirPage.SelectedValueIndex = 7 then
        modPath := 'source 2007 dedicated server\dod';

      InstallDirPage.Values[0] := GetMyAppPath('');
      Result := True;
      end;
    wpFinished:
      begin
      MsgBox('When you join your server, it is recommended that you run the admin tutorial by typing admintut in the console.' #13#13 'The tutorial helps new admins set themselves up as admin and helps them learn the commands. The tutorial can only be done once.', mbInformation, MB_OK);
      Result := True;
      end
  else
    Result := True;
  end;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
  MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result := 'Destination path:' + NewLine + Space + GetFinalInstallPath('');
end;












