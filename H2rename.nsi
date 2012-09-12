/*
	This file is part of H2rename.

	Copyright (C) 2009 by Harald Bögeholz / c't Magazin für Computertechnik
	www.ctmagazin.de

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

OutFile "release\H2rename-setup.exe"
Name "H2rename"
RequestExecutionLevel admin
InstallDir $PROGRAMFILES\H2rename
SetCompressor /SOLID lzma
LicenseData license.txt

Page license
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "Installer"
	SetOutPath $INSTDIR
	SetShellVarContext all
	File release\H2rename.exe
	File $%QTDIR%\bin\QtCore4.dll
	File $%QTDIR%\bin\QtGui4.dll
	File $%QTDIR%\..\mingw\bin\mingwm10.dll
	File COPYING
	WriteUninstaller uninstall.exe
	CreateShortCut "$SMPROGRAMS\H2rename.lnk" "$INSTDIR\H2rename.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "DisplayName" "H2rename"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "Publisher" "c't Magazin für Computertechnik"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "URLInfoAbout" "http://www.ctmagazin.de"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "DisplayVersion" "0.7.4"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "VersionMajor" 0
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "VersionMinor" 7
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename" "NoRepair" 1
SectionEnd

Section "un.Installer"
	SetShellVarContext all
	Delete $INSTDIR\uninstall.exe
	Delete $INSTDIR\H2rename.exe
	Delete $INSTDIR\QtCore4.dll
	Delete $INSTDIR\QtGui4.dll
	Delete $INSTDIR\mingwm10.dll
	Delete $INSTDIR\COPYING
	RMDir $INSTDIR
	Delete $SMPROGRAMS\H2rename.lnk
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\H2rename"
SectionEnd
