# =====================================================================
# Amethyst IDE — PowerShell Windows Build & Packaging Script
# Developer: Atimenka
# Repository: https://github.com/Atimenka/Amethyst
# =====================================================================

$ErrorActionPreference = "Stop"

Write-Host "====================================================================" -ForegroundColor Cyan
Write-Host "       Amethyst IDE — Windows Installer & Deployment Tool" -ForegroundColor Cyan
Write-Host "       Developer: Atimenka" -ForegroundColor Yellow
Write-Host "====================================================================" -ForegroundColor Cyan

$RepoRoot = Resolve-Path "$PSScriptRoot\..\.."
Set-Location $RepoRoot

# 1. Locate CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "CMake is not installed or not in PATH. Download from https://cmake.org"
}

# 2. Locate windeployqt
$windeployqt = (Get-Command windeployqt -ErrorAction SilentlyContinue)?.Source

if (-not $windeployqt) {
    # Check default Qt6 paths
    $qtPaths = Get-ChildItem "C:\Qt\6.*" -ErrorAction SilentlyContinue | ForEach-Object {
        Get-ChildItem "$($_.FullName)\mingw_*, $($_.FullName)\msvc*" -ErrorAction SilentlyContinue
    }
    foreach ($p in $qtPaths) {
        $candidate = "$($p.FullName)\bin\windeployqt.exe"
        if (Test-Path $candidate) {
            $windeployqt = $candidate
            $env:Path = "$($p.FullName)\bin;" + $env:Path
            break
        }
    }
}

if (-not $windeployqt -and (Test-Path "C:\msys64\mingw64\bin\windeployqt.exe")) {
    $windeployqt = "C:\msys64\mingw64\bin\windeployqt.exe"
    $env:Path = "C:\msys64\mingw64\bin;" + $env:Path
}

if (-not $windeployqt) {
    Write-Error "windeployqt.exe not found! Please install Qt6 and ensure bin\ is in PATH."
}
Write-Host "[OK] Using windeployqt: $windeployqt" -ForegroundColor Green

# 3. Locate Inno Setup
$iscc = (Get-Command iscc -ErrorAction SilentlyContinue)?.Source
if (-not $iscc) {
    $isccCandidates = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
    )
    foreach ($candidate in $isccCandidates) {
        if (Test-Path $candidate) {
            $iscc = $candidate
            break
        }
    }
}

# 4. Build in Release Mode
Write-Host "`n[1/4] Configuring CMake..." -ForegroundColor Yellow
cmake -B build_win -S src -DCMAKE_BUILD_TYPE=Release

Write-Host "`n[2/4] Building Amethyst (Release)..." -ForegroundColor Yellow
cmake --build build_win --config Release --parallel

$exePath = $null
if (Test-Path "build_win\Release\amethyst.exe") {
    $exePath = "build_win\Release\amethyst.exe"
} elseif (Test-Path "build_win\amethyst.exe") {
    $exePath = "build_win\amethyst.exe"
} else {
    Write-Error "Could not find built amethyst.exe!"
}
Write-Host "[OK] Found executable: $exePath" -ForegroundColor Green

# 5. Staging Bundle
Write-Host "`n[3/4] Staging portable bundle and deploying Qt6 DLLs..." -ForegroundColor Yellow
$bundleDir = "$RepoRoot\packaging\windows\bundle"
if (Test-Path $bundleDir) {
    Remove-Item -Recurse -Force $bundleDir
}
New-Item -ItemType Directory -Path $bundleDir | Out-Null

Copy-Item $exePath -Destination "$bundleDir\amethyst.exe"

& $windeployqt --release --compiler-runtime "$bundleDir\amethyst.exe"

# Copy MinGW runtime DLLs if available
@("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll") | ForEach-Object {
    $dllCmd = (Get-Command $_ -ErrorAction SilentlyContinue)?.Source
    if ($dllCmd -and -not (Test-Path "$bundleDir\$_")) {
        Copy-Item $dllCmd -Destination "$bundleDir\$_"
    }
}
Write-Host "[OK] Bundle successfully created in $bundleDir" -ForegroundColor Green

# 6. Build Inno Setup Installer
if ($iscc) {
    Write-Host "`n[4/4] Compiling Windows Installer (ISCC)..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path "$RepoRoot\dist" | Out-Null
    & $iscc "$RepoRoot\packaging\windows\amethyst_setup.iss"
    Write-Host "`n[SUCCESS] Setup installer created at: dist\Amethyst_Setup_x64.exe" -ForegroundColor Green
    Invoke-Item "$RepoRoot\dist"
} else {
    Write-Host "`n[INFO] Inno Setup (ISCC.exe) not found. Created portable bundle at: $bundleDir" -ForegroundColor Yellow
    Write-Host "To create a Setup .exe, install Inno Setup 6 from https://jrsoftware.org/isdl.php" -ForegroundColor Gray
    Invoke-Item $bundleDir
}
