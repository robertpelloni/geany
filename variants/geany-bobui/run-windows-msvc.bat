@echo off
setlocal
set "BOBUI_RUNTIME=%~dp0..\..\build\bobui-install\bin"
set "BOBUI_EXE=%~dp0..\..\build\geany-bobui\geany-bobui-search-studio.exe"
if not exist "%BOBUI_EXE%" (
  echo [geany-bobui] executable not found: %BOBUI_EXE%
  exit /b 1
)
if exist "%BOBUI_RUNTIME%" set "PATH=%BOBUI_RUNTIME%;%PATH%"
"%BOBUI_EXE%" %*
