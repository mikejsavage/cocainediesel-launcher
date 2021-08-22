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
!ifdef ONLY_WRITE_UNINSTALLER
	WriteUninstaller "$EXEDIR\..\release\uninstall.exe"
	SetErrorLevel 0
	Quit
!endif

	# Install stuff
	SetOutPath $INSTDIR
	File ..\release\cocainediesel.exe
	File ..\release\headlessupdater.exe
	File ..\release\elevate_for_update.exe
	File licenses.txt
	File apache-2.0.txt
	File gnu.txt
	File LGPL2.txt

	# Start menu shortcut
	CreateShortCut "$SMPROGRAMS\Cocaine Diesel.lnk" "$INSTDIR\cocainediesel.exe"

	# Uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"

	SetRegView 64

	# Uninstall registry keys
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

	# diesel:// registry keys
	WriteRegStr HKCR "diesel" "" "URL:diesel protocl"
	WriteRegStr HKCR "diesel" "URL Protocol" ""
	WriteRegStr HKCR "diesel\DefaultIcon" "" "$INSTDIR\cocainediesel.exe"
	WriteRegStr HKCR "diesel\Shell\Open\Command" "" "$\"$INSTDIR\client.exe$\" +set fs_basepath $\"$INSTDIR$\" +connect $\"%1$\""

	# Demo registry keys
	WriteRegStr HKCR ".cddemo" "" "Cocaine Diesel Demo"
	WriteRegStr HKCR "Cocaine Diesel Demo\DefaultIcon" "" "$INSTDIR\cocainediesel.exe"
	WriteRegStr HKCR "Cocaine Diesel Demo\Shell\Open\Command" "" "$\"$INSTDIR\client.exe$\" +set fs_basepath $\"$INSTDIR$\" +demo $\"%1$\""
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
	Delete "$INSTDIR\headlessupdater.exe"
	Delete "$INSTDIR\cocainediesel.exe.old"
	Delete "$INSTDIR\headlessupdater.exe.old"
	Delete "$INSTDIR\elevate_for_update.exe"
	Delete "$INSTDIR\bc4.exe"
	Delete "$INSTDIR\bc4"
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
	Delete "$SMPROGRAMS\Cocaine Diesel.lnk"

	# Registry keys
	SetRegView 64
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CocaineDiesel"
	DeleteRegKey HKCR "diesel"
	DeleteRegKey HKCR ".cddemo"
	DeleteRegKey HKCR "Cocaine Diesel Demo"
SectionEnd
