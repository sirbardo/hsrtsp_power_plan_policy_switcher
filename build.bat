@echo off
REM Build script for Short Thread Policy Switcher
REM Requires Visual Studio or MinGW-w64

REM Try MSVC first (cl.exe)
where cl >nul 2>&1
if %errorlevel%==0 (
    echo Building with MSVC...
    cl /O2 /W3 policy_switcher.c /Fe:policy_switcher.exe user32.lib PowrProf.lib
    goto :done
)

REM Try MinGW-w64 (gcc)
where gcc >nul 2>&1
if %errorlevel%==0 (
    echo Building with GCC...
    gcc -O2 -Wall -o policy_switcher.exe policy_switcher.c -luser32 -lpowrprof
    goto :done
)

echo ERROR: No C compiler found!
echo Please install Visual Studio or MinGW-w64
exit /b 1

:done
if exist policy_switcher.exe (
    echo.
    echo Build successful: policy_switcher.exe
    echo.
    echo NOTE: Run as Administrator to modify power settings
) else (
    echo Build failed!
)
