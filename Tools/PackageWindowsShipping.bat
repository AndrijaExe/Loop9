@echo off
setlocal
rem Package Windows Shipping without loading Unreal MCP in the cooker.
rem The editor already binds 127.0.0.1:8000; a second UnrealEditor-Cmd that
rem auto-starts MCP logs an Error and UAT treats a successful cook as failure.
set "ENGINE=D:\UE5.8\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat"
set "PROJECT=%~dp0..\Loop9.uproject"
set "OUT=D:\UE Course\Loop 9 AI\Builds\Alfa"

if not exist "%ENGINE%" (
	echo Missing RunUAT.bat: %ENGINE%
	exit /b 1
)

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
