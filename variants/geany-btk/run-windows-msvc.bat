@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..\..") do set "REPO_ROOT=%%~fI"

set "BTK_BIN=%REPO_ROOT%\build\btk-install\bin"
set "APP_EXE=%REPO_ROOT%\build\geany-btk-package3\geany-btk-search-studio.exe"

if not exist "%BTK_BIN%" (
  echo BTK runtime directory not found: %BTK_BIN%
  exit /b 1
)

if not exist "%APP_EXE%" (
  echo Geany BTK variant executable not found: %APP_EXE%
  exit /b 1
)

set "PATH=%BTK_BIN%;%PATH%"
start "Geany BTK Search Studio" "%APP_EXE%"
