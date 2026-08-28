@echo off
setlocal
rem Package Windows Development for store-review / QA builds.
rem
rem Shipping compiles out every debug console command (EndingSetup, AnomalyForce,
rem AudioStatus). A reviewer who has to reach all six endings needs those, so the
rem review build must be Development. Never ship this config to the default
rem branch: it is slower and leaves the console open to players.
rem
rem Usage:
rem   PackageWindowsDevelopment.bat            -> Builds\Debug
rem   PackageWindowsDevelopment.bat ReviewA    -> Builds\ReviewA
rem
rem Upload it to a passworded branch only. See
rem Marketing\Steam\VALVE_REVIEW_REPLY.md for the branch and reply workflow.
set "ENGINE=D:\UE5.8\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat"
set "PROJECT=%~dp0..\Loop9.uproject"
set "FOLDER=%~1"
if "%FOLDER%"=="" set "FOLDER=Debug"
set "OUT=D:\UE Course\Loop 9 AI\Builds\%FOLDER%"

if not exist "%ENGINE%" (
	echo Missing RunUAT.bat: %ENGINE%
	exit /b 1
)

echo Archiving Development build to: %OUT%
echo Shipping drops live elsewhere; do not point SteamPipe default branch here.

call "%ENGINE%" BuildCookRun ^
	-project="%PROJECT%" ^
	-platform=Win64 ^
	-clientconfig=Development ^
	-target=Loop9 ^
	-cook -build -stage -pak -iostore -compressed -prereqs -package ^
	-archive -archivedirectory="%OUT%" ^
	-nodebuginfo ^
	-AdditionalCookerOptions="-DisablePlugins=ModelContextProtocol"

echo.
echo Exit code: %ERRORLEVEL%
exit /b %ERRORLEVEL%
