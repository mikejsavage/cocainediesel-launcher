!include "MUI2.nsh"

Name "Medfall"
Outfile "MedfallInstaller.exe"

InstallDir "$PROGRAMFILES64\Medfall"
RequestExecutionLevel admin

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

Section "Install" SectionInstall
	# Install stuff
	SetOutPath $INSTDIR
	File release\launch.exe
	File release\elevate_for_update.exe

	# Start menu shortcut
	CreateDirectory "$SMPROGRAMS\Medfall"
	CreateShortCut "$SMPROGRAMS\Medfall\Medfall.lnk" "$INSTDIR\launch.exe"

	# Uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"

	# Registry keys
	SetRegView 64
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall" "DisplayName" "Medfall"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall" "DisplayIcon" "$INSTDIR\logo.ico"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall" "Publisher" "Medfall Yes Son"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall" "DisplayVersion" "0.0.0.0"
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall" "NoModify" 1
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall" "NoRepair" 1
	
	SectionGetSize ${SectionInstall} $0
	IntFmt $1 "0x%08X" $0
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall" "EstimatedSize" $1
SectionEnd

Section "Uninstall"
	# Files
	Delete "$INSTDIR\medfall.exe"
	Delete "$INSTDIR\launch.exe"
	Delete "$INSTDIR\launch.exe.old"
	Delete "$INSTDIR\elevate_for_update.exe"
	Delete "$INSTDIR\uninstall.exe"
	Delete "$INSTDIR\version.txt"
	Delete "$INSTDIR\manifest.txt"

	Delete "$INSTDIR\LiberationSans-Regular.ttf"

	RMDir /r "$INSTDIR\logs"
	RMDir /r "$INSTDIR\models"
	RMDir /r "$INSTDIR\shaders"
	RMDir /r "$INSTDIR\terrains"
	RMDir /r "$INSTDIR\update"
	RMDir "$INSTDIR"

	# Start menu shortcut
	Delete "$SMPROGRAMS\Medfall\Medfall.lnk"
	RMDir "$SMPROGRAMS\Medfall"

	# Registry keys
	SetRegView 64
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Medfall"
SectionEnd
