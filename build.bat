@echo off

pushd %~dp0

if "%SHELLSETUP%"=="" (
    call shell.bat
) ELSE (
    echo Skipping setup
)

REM echo premake5.exe --file=premake5.lua vs2015
REM premake5.exe --file=premake5.lua vs2015

REM Skip this while premake takes less than half a second?
IF NOT EXIST "V3.sln" (
    premake5.exe --file=premake5.lua vs2022
    echo Running Premake
) ELSE (
    REM echo Skipping premake
)
echo Running msbuild
REM msbuild /t:MutaGen /nologo /verbosity:minimal build\Siege.sln
msbuild /t:V3 /nologo /verbosity:minimal -p:Configuration=Debug V3.sln


popd

