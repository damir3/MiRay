@echo off
setlocal enabledelayedexpansion

set ARCH=%1
if "%ARCH%"=="" set ARCH=64
if "%ARCH%"=="intel" set ARCH=64
if "%ARCH%"=="x64" set ARCH=64
if "%ARCH%"=="arm" set ARCH=arm64

set BIN_DIR=bin\%ARCH%
set APP_EXE=%BIN_DIR%\MiRay.exe

if not exist "%APP_EXE%" (
	echo Error: %APP_EXE% does not exist. Please build the project first.
	exit /b 1
)

echo ==^> Preparing Windows Standalone Application: %BIN_DIR%

:: 1. Copy qt.conf to bin directory for QML runtime and plugin resolution
if exist "tools\win\qt.conf" (
	echo ==^> Deploying qt.conf...
	copy /y "tools\win\qt.conf" "%BIN_DIR%\qt.conf" >nul
)

:: 2. Ensure application resource directories (Preview, Library) are present
if exist "Resources\Preview" (
	echo ==^> Syncing Preview resources...
	if not exist "%BIN_DIR%\Preview" mkdir "%BIN_DIR%\Preview"
	xcopy /e /i /y "Resources\Preview" "%BIN_DIR%\Preview" >nul
)

if exist "Resources\Library" (
	echo ==^> Syncing Library resources...
	if not exist "%BIN_DIR%\Library" mkdir "%BIN_DIR%\Library"
	xcopy /e /i /y "Resources\Library" "%BIN_DIR%\Library" >nul
)

:: 3. Clean up stale/compiled QML cache files
if exist "%BIN_DIR%\qml" (
	del /s /q /f "%BIN_DIR%\qml\*.qmlc" >nul 2>nul
)

:: 4. Verification of required components
echo ==^> Verifying deployment...
if not exist "%BIN_DIR%\platforms\qwindows.dll" (
	echo Warning: %BIN_DIR%\platforms\qwindows.dll not found. Qt GUI initialization might fail.
)
if not exist "%BIN_DIR%\qt.conf" (
	echo Warning: %BIN_DIR%\qt.conf not found. QML imports might fail to resolve.
)

echo ==^> Standalone Windows application ready at %APP_EXE%
