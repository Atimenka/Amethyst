@echo off
setlocal enabledelayedexpansion
title Amethyst Windows Build ^& Packaging Script
color 0B

echo ====================================================================
echo        Amethyst IDE — Windows Standalone Installer Builder
echo        Developer: Atimenka
echo ====================================================================
echo.

:: 1. Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake is not found in PATH!
    echo Please install CMake from https://cmake.org/download/ and add to PATH.
    pause
    exit /b 1
)

:: 2. Check for windeployqt
set "WINDEPLOYQT_BIN="
where windeployqt >nul 2>nul
if %errorlevel% equ 0 (
    for /f "delims=" %%I in ('where windeployqt') do (
        if not defined WINDEPLOYQT_BIN set "WINDEPLOYQT_BIN=%%I"
    )
)

if not defined WINDEPLOYQT_BIN (
    echo [WARNING] windeployqt was not found in PATH.
    echo Searching common Qt6 installation paths...
    for /d %%D in (C:\Qt\6.*) do (
        for /d %%C in (%%D\mingw_* %%D\msvc*) do (
            if exist "%%C\bin\windeployqt.exe" (
                set "WINDEPLOYQT_BIN=%%C\bin\windeployqt.exe"
                set "PATH=%%C\bin;!PATH!"
            )
        )
    )
)

if not defined WINDEPLOYQT_BIN (
    if exist "C:\msys64\mingw64\bin\windeployqt.exe" (
        set "WINDEPLOYQT_BIN=C:\msys64\mingw64\bin\windeployqt.exe"
        set "PATH=C:\msys64\mingw64\bin;!PATH!"
    )
)

if not defined WINDEPLOYQT_BIN (
    echo [ERROR] windeployqt.exe could not be found!
    echo Please ensure Qt6 is installed and its bin directory is in your PATH.
    pause
    exit /b 1
)
echo [OK] Using windeployqt: !WINDEPLOYQT_BIN!

:: 3. Check for Inno Setup (ISCC.exe)
set "ISCC_BIN="
where iscc >nul 2>nul
if %errorlevel% equ 0 (
    set "ISCC_BIN=iscc"
) else (
    if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
        set "ISCC_BIN=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
    ) else if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" (
        set "ISCC_BIN=%ProgramFiles%\Inno Setup 6\ISCC.exe"
    )
)

if not defined ISCC_BIN (
    echo [WARNING] Inno Setup Compiler (ISCC.exe) not found!
    echo Setup installer cannot be generated automatically, but a portable folder will be created.
    echo Download Inno Setup from: https://jrsoftware.org/isdl.php
)

:: 4. Navigate to Repository Root
cd /d "%~dp0..\.."
echo [INFO] Working directory: %CD%

:: 5. Build Amethyst with CMake (Release Mode)
echo.
echo [1/4] Configuring CMake project in Release mode...
cmake -B build_win -S src -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [2/4] Compiling Amethyst...
cmake --build build_win --config Release --parallel
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

:: Locate amethyst.exe
set "AMETHYST_EXE="
if exist "build_win\Release\amethyst.exe" (
    set "AMETHYST_EXE=build_win\Release\amethyst.exe"
) else if exist "build_win\amethyst.exe" (
    set "AMETHYST_EXE=build_win\amethyst.exe"
)

if not defined AMETHYST_EXE (
    echo [ERROR] Could not find compiled amethyst.exe!
    pause
    exit /b 1
)
echo [OK] Found executable: !AMETHYST_EXE!

:: 6. Stage Application Bundle & Deploy Qt Dependencies
echo.
echo [3/4] Preparing portable bundle with windeployqt...
set "BUNDLE_DIR=packaging\windows\bundle"
if exist "%BUNDLE_DIR%" rmdir /s /q "%BUNDLE_DIR%"
mkdir "%BUNDLE_DIR%"

copy /y "!AMETHYST_EXE!" "%BUNDLE_DIR%\amethyst.exe" >nul

echo Running windeployqt to collect Qt6 DLLs and plugins...
"!WINDEPLOYQT_BIN!" --release --compiler-runtime "%BUNDLE_DIR%\amethyst.exe"

:: Copy MinGW runtime DLLs if applicable
for %%F in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
    where %%F >nul 2>nul
    if !errorlevel! equ 0 (
        for /f "delims=" %%I in ('where %%F') do (
            if not exist "%BUNDLE_DIR%\%%F" copy /y "%%I" "%BUNDLE_DIR%\" >nul
        )
    )
)

echo [OK] Portable bundle ready at %BUNDLE_DIR%

:: 7. Build Inno Setup Installer
if defined ISCC_BIN (
    echo.
    echo [4/4] Building Inno Setup installer...
    if not exist "dist" mkdir "dist"
    "!ISCC_BIN!" "packaging\windows\amethyst_setup.iss"
    if !errorlevel! equ 0 (
        echo.
        echo ====================================================================
        echo [SUCCESS] Installer successfully generated!
        echo Location: dist\Amethyst_Setup_x64.exe
        echo ====================================================================
        explorer "dist"
    ) else (
        echo [ERROR] Inno Setup compilation failed!
    )
) else (
    echo.
    echo ====================================================================
    echo [NOTE] Portable standalone folder is ready at:
    echo        %CD%\%BUNDLE_DIR%
    echo (You can run %BUNDLE_DIR%\amethyst.exe directly without installing Qt!)
    echo ====================================================================
    explorer "%BUNDLE_DIR%"
)

pause
