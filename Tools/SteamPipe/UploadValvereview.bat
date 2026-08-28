@echo off
setlocal
rem Upload Builds\v1.0.2Dev\Windows to SteamPipe and set live on valvereview.
rem Playtest VDFs stay pointed at v1.0.2 Shipping. Do not use this for default/playtest.

set "STEAMCMD=D:\steamcmd\steamcmd.exe"
set "VDF=%~dp0app_build_valvereview.vdf"
set "CONTENT=D:\UE Course\Loop 9 AI\Builds\v1.0.2Dev\Windows"

if not exist "%STEAMCMD%" (
	echo Missing SteamCMD: %STEAMCMD%
	exit /b 1
)
if not exist "%CONTENT%\Loop9.exe" (
	echo Missing Loop9.exe in %CONTENT%
	exit /b 1
)
if exist "%CONTENT%\steam_appid.txt" (
	echo Do not upload steam_appid.txt. Delete it from the Windows folder first.
	exit /b 1
)

if "%STEAM_USER%"=="" (
	set /p STEAM_USER=Steamworks username:
)
if "%STEAM_USER%"=="" (
	echo STEAM_USER is required.
	exit /b 1
)

echo.
echo Logging in as %STEAM_USER% and uploading depot 4982261 to branch valvereview.
echo Steam Guard / password prompts appear in this window.
echo.

"%STEAMCMD%" +login %STEAM_USER% +run_app_build "%VDF%" +quit
echo.
echo Exit code: %ERRORLEVEL%
exit /b %ERRORLEVEL%
