@echo off
setlocal enabledelayedexpansion
REM Build script for Short Thread Policy Switcher (GUI Version)
REM Requires Visual Studio or MinGW-w64

REM Try MSVC first (cl.exe)
where cl >nul 2>&1
if !errorlevel!==0 (
    echo Building GUI version with MSVC...
    cl /O2 /W3 policy_switcher_gui.c /Fe:policy_switcher_gui.exe user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:policy_switcher_gui.manifest
    goto :done
)

REM Try MinGW-w64 (gcc)
where gcc >nul 2>&1
if !errorlevel!==0 (
    echo Building GUI version with GCC...

    REM Delete old exe to ensure we detect build failures
    if exist policy_switcher_gui.exe del policy_switcher_gui.exe

    echo Compiling resource...
    windres -o manifest.o policy_switcher_gui.rc
    if !errorlevel! neq 0 (
        echo ERROR: windres failed
        exit /b 1
    )

    echo Compiling and linking...
    gcc -O2 -Wall -Werror -mwindows -o policy_switcher_gui.exe policy_switcher_gui.c manifest.o -luser32 -lgdi32
    set GCC_EXIT=!errorlevel!
    del manifest.o 2>nul

    if !GCC_EXIT! neq 0 (
        echo.
        echo ERROR: Compilation failed with errors above
        echo.
        exit /b 1
    )
    goto :done
)

echo ERROR: No C compiler found!
echo Please install Visual Studio or MinGW-w64
exit /b 1

:done
if exist policy_switcher_gui.exe (
    echo.
    echo Build successful: policy_switcher_gui.exe
    echo.
    echo NOTE: Run as Administrator for powercfg to work properly
) else (
    echo Build failed!
)
