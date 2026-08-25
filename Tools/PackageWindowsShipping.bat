@echo off
setlocal
rem Package Windows Shipping without loading Unreal MCP in the cooker.
rem The editor already binds 127.0.0.1:8000; a second UnrealEditor-Cmd that
rem auto-starts MCP logs an Error and UAT treats a successful cook as failure.
rem
rem Usage:
rem   PackageWindowsShipping.bat              -> Builds\Feature (WIP)
rem   PackageWindowsShipping.bat v1.0.1       -> Builds\v1.0.1
rem
rem Do not cook into Builds\v1.0.0 unless that folder is explicitly requested.
rem v1.0.0 is the current fallback / last frozen Shipping drop.
set "ENGINE=D:\UE5.8\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat"
set "PROJECT=%~dp0..\Loop9.uproject"
set "FOLDER=%~1"
if "%FOLDER%"=="" set "FOLDER=Feature"
set "OUT=D:\UE Course\Loop 9 AI\Builds\%FOLDER%"

if not exist "%ENGINE%" (
	echo Missing RunUAT.bat: %ENGINE%
	exit /b 1
)

echo Archiving to: %OUT%
echo Current fallback (leave alone unless asked): D:\UE Course\Loop 9 AI\Builds\v1.0.0

call "%ENGINE%" BuildCookRun ^
	-project="%PROJECT%" ^
	-platform=Win64 ^
	-clientconfig=Shipping ^
	-target=Loop9 ^
	-cook -build -stage -pak -iostore -compressed -prereqs -package ^
	-archive -archivedirectory="%OUT%" ^
	-nodebuginfo ^
	-AdditionalCookerOptions="-DisablePlugins=ModelContextProtocol"

echo.
echo Exit code: %ERRORLEVEL%
exit /b %ERRORLEVEL%
