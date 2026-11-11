@echo off
REM Mars Hello World - Windows build: generate assets, compile, cook, stage, package.
REM Usage:  Scripts\BuildWin.bat  [UE_ROOT]
REM Default UE_ROOT: C:\Program Files\Epic Games\UE_5.8
setlocal
set "UE_ROOT=%~1"
if "%UE_ROOT%"=="" set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=%~dp0..\MarsHelloWorld.uproject"
set "OUT=%~dp0..\Build\Windows"

if not exist "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" (
  echo Unreal Engine not found at "%UE_ROOT%". Pass the engine folder as the first argument.
  exit /b 1
)

echo === 1/2 Generating voices and materials (mars_setup.py) ===
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT%" -run=pythonscript -script="mars_setup.py" -unattended -nop4 -nosplash
if errorlevel 1 echo WARNING: asset setup failed; the game still runs with fallbacks.

echo === 2/2 BuildCookRun ===
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="%PROJECT%" -noP4 -platform=Win64 -clientconfig=Development ^
  -build -cook -stage -pak -iostore -package -archive -archivedirectory="%OUT%" -utf8output
if errorlevel 1 exit /b 1

echo Done. Run: "%OUT%\Windows\MarsHelloWorld.exe"
endlocal
