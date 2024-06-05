@echo off


if "%SHELLSETUP%"=="" (
    echo Running shell.bat
    set SHELLSETUP=""
	REM "%VS140COMNTOOLS%"vsvars32.bat x86_amd64
    REM

    IF EXIST "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\"vcvarsall.bat x86_amd64
    ) ELSE IF EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        REM VS 2019 already reports this info in vcvarsall.bat, no need to output this
        REM echo Setting up shell for x86_64 compiling with VS 2019
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\"vcvarsall.bat x86_amd64
    ) ELSE IF EXIST "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\"vcvarsall.bat (
        echo Setting up shell for x86_64 compiling with VS 2015
        call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\"vcvarsall.bat x86_amd64
    ) ELSE (
        echo Unable to find Visual Studio installation
        set SHELLSETUP=
    )

) ELSE (
    REM echo Skipping setup
)
