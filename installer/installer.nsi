!include "MUI2.nsh"

Name "Cocaine Diesel"
Outfile "CocaineDieselInstaller.exe"

!define MUI_ICON "icon.ico"
!define MUI_UNICON "icon.ico"

InstallDir "$PROGRAMFILES64\Cocaine Diesel"
RequestExecutionLevel admin

SetCompressor lzma

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

Section "Install" SectionInstall
	# Install stuff
	SetOutPath $INSTDIR
	File ..\release\cocainediesel.exe
	File ..\release\elevate_for_update.exe
	File licenses.txt
	File apache-2.0.txt
	File gnu.txt
	File LGPL2.txt

	# Start menu shortcut
	CreateDirectory "$SMPROGRAMS\Cocaine Diesel"
	CreateShortCut "$SMPROGRAMS\Cocaine Diesel\Cocaine Diesel.lnk" "$INSTDIR\cocainediesel.exe"

	# Uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"

	# Registry keys
	SetRegView 64
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel" "DisplayName" "Cocaine Diesel"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel" "DisplayIcon" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel" "Publisher" "Aha Cheers"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel" "DisplayVersion" "0.0.0.0"
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel" "NoModify" 1
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel" "NoRepair" 1
	
	SectionGetSize ${SectionInstall} $0
	IntFmt $1 "0x%08X" $0
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel" "EstimatedSize" $1
SectionEnd

Function .onInstSuccess
	Exec "cocainediesel.exe"
FunctionEnd

Section "Uninstall"
	# Files
	Delete "$INSTDIR\client.exe"
	Delete "$INSTDIR\SDL2.dll"
	Delete "$INSTDIR\server.exe"
	Delete "$INSTDIR\cocainediesel.exe"
	Delete "$INSTDIR\cocainediesel.exe.old"
	Delete "$INSTDIR\elevate_for_update.exe"
	Delete "$INSTDIR\uninstall.exe"
	Delete "$INSTDIR\licenses.txt"
	Delete "$INSTDIR\apache-2.0.txt"
	Delete "$INSTDIR\gnu.txt"
	Delete "$INSTDIR\LGPL2.txt"
	Delete "$INSTDIR\version.txt"
	Delete "$INSTDIR\manifest.txt"

	RMDir /r "$INSTDIR\base"
	RMDir /r "$INSTDIR\libs"
	RMDir "$INSTDIR"

	# Start menu shortcut
	Delete "$SMPROGRAMS\Cocaine Diesel\Cocaine Diesel.lnk"
	RMDir "$SMPROGRAMS\Cocaine Diesel"

	# Registry keys
	SetRegView 64
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel"
SectionEnd
