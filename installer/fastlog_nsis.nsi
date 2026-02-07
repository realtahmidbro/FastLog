; FastLog NSIS Installer
; Professioneller Windows Installer

!include "MUI2.nsh"
!include "WinMessages.nsh"
!include "WordFunc.nsh"

; ============================================================================
; Metadata
; ============================================================================
!define PRODUCT_NAME "FastLog"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "AGDNoob"
!define PRODUCT_WEB_SITE "https://github.com/AGDNoob/fastlog"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\fastlog.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; ============================================================================
; Installer Attributes
; ============================================================================
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "..\dist\FastLog_Setup_v${PRODUCT_VERSION}_NSIS.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
RequestExecutionLevel admin
ShowInstDetails show
ShowUnInstDetails show
SetCompressor /SOLID lzma

; Version Info - das erscheint in den Dateieigenschaften
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "LegalCopyright" "Copyright (c) 2026 ${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Installer"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"

; ============================================================================
; Modern UI Configuration
; ============================================================================
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp"

; Welcome page
!define MUI_WELCOMEPAGE_TITLE "Welcome to ${PRODUCT_NAME} Setup"
!define MUI_WELCOMEPAGE_TEXT "This wizard will install ${PRODUCT_NAME} v${PRODUCT_VERSION} on your computer.$\r$\n$\r$\nFastLog is an ultra high-performance text file analyzer:$\r$\n$\r$\n• Over 1 GB/s throughput$\r$\n• SIMD optimized (AVX2/SSE2)$\r$\n• Multi-threaded$\r$\n• Encoding detection$\r$\n• Delimiter analysis$\r$\n$\r$\nClick Next to continue."

; ============================================================================
; Pages
; ============================================================================
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Open Command Prompt with FastLog"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchCmd"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "German"

; ============================================================================
; Installer Section
; ============================================================================
Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    SetOverwrite on
    
    ; Hauptdatei
    File "..\build\fastlog.exe"
    
    ; Start Menu Shortcuts
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\fastlog.exe" "--help"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Check for Updates.lnk" "$INSTDIR\fastlog.exe" "--update"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninst.exe"
    
    ; PATH hinzufügen via Registry
    ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    StrCpy $0 "$0;$INSTDIR"
    WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$0"
    ; Broadcast WM_SETTINGCHANGE damit Explorer PATH neu lädt
    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
    
    ; Registry
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\fastlog.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\fastlog.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
    
    ; Uninstaller erstellen
    WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd

; ============================================================================
; Uninstaller Section
; ============================================================================
Section Uninstall
    ; PATH entfernen - lese aktuellen PATH und entferne $INSTDIR
    ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    ${WordReplace} "$0" ";$INSTDIR" "" "+" $0
    WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$0"
    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
    
    ; Dateien löschen
    Delete "$INSTDIR\fastlog.exe"
    Delete "$INSTDIR\uninst.exe"
    
    ; Shortcuts löschen
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Check for Updates.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
    
    ; Installationsordner löschen
    RMDir "$INSTDIR"
    
    ; Registry aufräumen
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
SectionEnd

; ============================================================================
; Functions
; ============================================================================
Function LaunchCmd
    Exec 'cmd.exe /k "fastlog --help"'
FunctionEnd

Function .onInit
    ; Sprache automatisch wählen
    !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd
