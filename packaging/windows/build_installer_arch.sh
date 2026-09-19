#!/usr/bin/env bash
# =====================================================================
# Amethyst IDE — Arch Linux to Windows Installer Builder (Inno Setup 6)
# Developer: Atimenka
# Repository: https://github.com/Atimenka/Amethyst
# =====================================================================

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${CYAN}====================================================================${NC}"
echo -e "${CYAN}       Amethyst IDE — Windows Installer Builder on Arch Linux       ${NC}"
echo -e "${YELLOW}       Developer: Atimenka${NC}"
echo -e "${CYAN}====================================================================${NC}\n"

# 1. Determine Repository Root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

BUNDLE_DIR="$SCRIPT_DIR/bundle"
DIST_DIR="$REPO_ROOT/dist"

mkdir -p "$BUNDLE_DIR"
mkdir -p "$DIST_DIR"

# 2. Check WSL vs Pure Linux
IS_WSL=false
if grep -qi microsoft /proc/version 2>/dev/null; then
    IS_WSL=true
    echo -e "${GREEN}[INFO] Detected Arch Linux inside WSL (Windows Subsystem for Linux)!${NC}"
fi

# Ensure Qt6 manifest template exists if missing in host Arch Qt package
if [ -d "/usr/lib/cmake/Qt6" ] && [ ! -f "/usr/lib/cmake/Qt6/windows/app.exe.manifest.in" ]; then
    mkdir -p "/usr/lib/cmake/Qt6/windows" 2>/dev/null || true
    cat << 'EOF' > "/usr/lib/cmake/Qt6/windows/app.exe.manifest.in" 2>/dev/null || true
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity type="win32" name="@target@" version="1.0.0.0"/>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
    </application>
  </compatibility>
</assembly>
EOF
fi

# 3. Locate amethyst.exe
AMETHYST_EXE=""
for candidate in \
    "$BUNDLE_DIR/amethyst.exe" \
    "build_win/amethyst.exe" \
    "build_win/Release/amethyst.exe" \
    "build_mingw/amethyst.exe" \
    "/mnt/c/Amethyst/build_win/Release/amethyst.exe" \
    "/mnt/c/Amethyst/build_win/amethyst.exe" \
    "/mnt/c/Amethyst/packaging/windows/bundle/amethyst.exe"; do
    if [ -f "$candidate" ]; then
        AMETHYST_EXE="$candidate"
        break
    fi
done

# If amethyst.exe not found, try to compile
if [ -z "$AMETHYST_EXE" ]; then
    echo -e "${YELLOW}[INFO] Prebuilt amethyst.exe not found in staging.${NC}"

    # In WSL, try to build natively using Windows toolchain first
    if [ "$IS_WSL" = true ] && [ -x "/mnt/c/Windows/System32/cmd.exe" ]; then
        if /mnt/c/Windows/System32/cmd.exe /c "where cmake" >/dev/null 2>&1; then
            echo -e "${CYAN}[1/4] Found Windows CMake via WSL! Building amethyst.exe natively...${NC}"
            WIN_REPO_PATH="$(wslpath -w "$REPO_ROOT")"
            /mnt/c/Windows/System32/cmd.exe /c "cd /d \"$WIN_REPO_PATH\" && cmake -B build_win -S src -DCMAKE_BUILD_TYPE=Release && cmake --build build_win --config Release --parallel" || true
            if [ -f "build_win/Release/amethyst.exe" ]; then
                AMETHYST_EXE="build_win/Release/amethyst.exe"
            elif [ -f "build_win/amethyst.exe" ]; then
                AMETHYST_EXE="build_win/amethyst.exe"
            fi
        fi
    fi

    # If still not found, try cross-compiling with mingw-w64 on Arch
    if [ -z "$AMETHYST_EXE" ] && command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
        echo -e "${CYAN}[1/4] Compiling amethyst.exe with mingw-w64...${NC}"
        rm -rf build_mingw
        cmake -B build_mingw -S src \
            -DCMAKE_SYSTEM_NAME=Windows \
            -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
            -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
            -DCMAKE_BUILD_TYPE=Release \
            -DQT_NO_WINDOWS_APP_MANIFEST=TRUE
        cmake --build build_mingw --parallel "$(nproc)" || true
        if [ -f "build_mingw/amethyst.exe" ]; then
            AMETHYST_EXE="build_mingw/amethyst.exe"
        fi
    fi
fi

if [ -n "$AMETHYST_EXE" ] && [ -f "$AMETHYST_EXE" ]; then
    echo -e "${GREEN}[OK] Found amethyst.exe: $AMETHYST_EXE${NC}"
    cp -f "$AMETHYST_EXE" "$BUNDLE_DIR/amethyst.exe"
else
    echo -e "${RED}[ERROR] amethyst.exe не найден!${NC}"
    echo -e "${YELLOW}Для сборки Setup-инсталлятора необходим Windows-бинарник amethyst.exe.${NC}"
    echo -e "${YELLOW}Как его получить (выберите любой способ):${NC}"
    echo -e "  1. Запустите сборку в Windows через: ${CYAN}packaging\\windows\\build_installer.bat${NC}"
    echo -e "  2. Или скопируйте готовый amethyst.exe в: ${CYAN}$BUNDLE_DIR/amethyst.exe${NC}"
    echo -e "  3. Или установите Windows-версию Qt6 в Arch: ${CYAN}yay -S mingw-w64-qt6-base mingw-w64-qt6-svg${NC}"
    exit 1
fi

# 4. Deploy Qt6 Windows Runtime DLLs and Plugins
echo -e "\n${CYAN}[2/4] Gathering Qt6 Windows runtime DLLs and plugins...${NC}"
mkdir -p "$BUNDLE_DIR/platforms"
mkdir -p "$BUNDLE_DIR/imageformats"
mkdir -p "$BUNDLE_DIR/styles"

# In WSL, try windeployqt from Windows Qt installation
if [ "$IS_WSL" = true ]; then
    WIN_WINDEPLOYQT=""
    for qt_bin in /mnt/c/Qt/6.*/mingw_*/bin/windeployqt.exe /mnt/c/Qt/6.*/msvc*/bin/windeployqt.exe /mnt/c/msys64/mingw64/bin/windeployqt.exe; do
        if [ -f "$qt_bin" ]; then
            WIN_WINDEPLOYQT="$qt_bin"
            break
        fi
    done

    if [ -n "$WIN_WINDEPLOYQT" ]; then
        echo -e "${GREEN}[OK] Running Windows windeployqt via WSL: $WIN_WINDEPLOYQT${NC}"
        WIN_BUNDLE_WINPATH="$(wslpath -w "$BUNDLE_DIR/amethyst.exe")"
        "$WIN_WINDEPLOYQT" --release --compiler-runtime "$WIN_BUNDLE_WINPATH" || true
    fi
fi

# If on pure Arch with mingw-w64-qt6 installed
MINGW_PREFIX="/usr/x86_64-w64-mingw32"
if [ -d "$MINGW_PREFIX/bin" ]; then
    echo -e "${CYAN}Copying MinGW Qt6 DLLs from Arch system...${NC}"
    for dll in Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Svg.dll Qt6Concurrent.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
        if [ -f "$MINGW_PREFIX/bin/$dll" ]; then
            cp -f "$MINGW_PREFIX/bin/$dll" "$BUNDLE_DIR/"
        fi
    done
    if [ -f "$MINGW_PREFIX/lib/qt6/plugins/platforms/qwindows.dll" ]; then
        cp -f "$MINGW_PREFIX/lib/qt6/plugins/platforms/qwindows.dll" "$BUNDLE_DIR/platforms/"
    fi
    if [ -d "$MINGW_PREFIX/lib/qt6/plugins/imageformats" ]; then
        cp -rf "$MINGW_PREFIX/lib/qt6/plugins/imageformats/"* "$BUNDLE_DIR/imageformats/"
    fi
fi

# Ensure icon is copied
cp -f "$REPO_ROOT/src/resources/icons/amethyst.ico" "$SCRIPT_DIR/amethyst.ico"

# 5. Locate or Install Inno Setup Compiler (ISCC)
echo -e "\n${CYAN}[3/4] Locating Inno Setup 6 compiler (ISCC)...${NC}"
ISCC_CMD=""

# 5.1 Check Windows native ISCC in WSL
if [ "$IS_WSL" = true ]; then
    for iscc_path in "/mnt/c/Program Files (x86)/Inno Setup 6/ISCC.exe" "/mnt/c/Program Files/Inno Setup 6/ISCC.exe"; do
        if [ -f "$iscc_path" ]; then
            ISCC_CMD="$iscc_path"
            echo -e "${GREEN}[OK] Found Windows Inno Setup via WSL: $ISCC_CMD${NC}"
            break
        fi
    done
fi

# 5.2 Check wine or native iscc on Arch
if [ -z "$ISCC_CMD" ]; then
    if command -v iscc >/dev/null 2>&1; then
        ISCC_CMD="iscc"
        echo -e "${GREEN}[OK] Found native iscc in PATH${NC}"
    elif command -v wine >/dev/null 2>&1; then
        # Check standard wine Inno Setup path
        WINE_ISCC="$HOME/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe"
        if [ -f "$WINE_ISCC" ]; then
            ISCC_CMD="wine \"$WINE_ISCC\""
            echo -e "${GREEN}[OK] Found Wine Inno Setup at $WINE_ISCC${NC}"
        else
            echo -e "${YELLOW}[INFO] Inno Setup 6 is not in Wine.${NC}"
            echo -e "Downloading Inno Setup 6 portable for Linux/Wine..."
            INNO_CACHE="$HOME/.cache/innosetup6"
            mkdir -p "$INNO_CACHE"
            if [ ! -f "$INNO_CACHE/ISCC.exe" ]; then
                SETUP_TMP="/tmp/innosetup.exe"
                curl -L "https://files.jrsoftware.org/is/6/innosetup-6.3.3.exe" -o "$SETUP_TMP"
                if command -v innoextract >/dev/null 2>&1; then
                    innoextract -d "$INNO_CACHE" -e "$SETUP_TMP"
                    # Move app files to root of cache if nested
                    if [ -d "$INNO_CACHE/app" ]; then
                        cp -rf "$INNO_CACHE/app/"* "$INNO_CACHE/"
                    fi
                else
                    echo -e "${YELLOW}Running silent installation into Wine...${NC}"
                    wine "$SETUP_TMP" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR="C:\\Program Files (x86)\\Inno Setup 6"
                fi
                rm -f "$SETUP_TMP"
            fi
            if [ -f "$INNO_CACHE/ISCC.exe" ]; then
                ISCC_CMD="wine \"$INNO_CACHE/ISCC.exe\""
            elif [ -f "$WINE_ISCC" ]; then
                ISCC_CMD="wine \"$WINE_ISCC\""
            fi
        fi
    fi
fi

# 6. Compile the Windows Installer
echo -e "\n${CYAN}[4/4] Compiling Windows Installer (Amethyst_Setup_x64.exe)...${NC}"

if [ -n "$ISCC_CMD" ]; then
    cd "$SCRIPT_DIR"
    if [ "$IS_WSL" = true ] && [[ "$ISCC_CMD" == /mnt/c/* ]]; then
        # Running Windows ISCC.exe from WSL requires Windows-style path
        WIN_ISS="$(wslpath -w "$SCRIPT_DIR/amethyst_setup.iss")"
        "$ISCC_CMD" "$WIN_ISS"
    elif [[ "$ISCC_CMD" == wine* ]]; then
        eval $ISCC_CMD amethyst_setup.iss
    else
        $ISCC_CMD amethyst_setup.iss
    fi

    if [ -f "$DIST_DIR/Amethyst_Setup_x64.exe" ]; then
        echo -e "\n${GREEN}====================================================================${NC}"
        echo -e "${GREEN}[SUCCESS] Setup installer successfully created!${NC}"
        echo -e "${GREEN}Location: $DIST_DIR/Amethyst_Setup_x64.exe${NC}"
        echo -e "${GREEN}====================================================================${NC}"
        ls -lh "$DIST_DIR/Amethyst_Setup_x64.exe"
    else
        echo -e "${RED}[ERROR] ISCC executed, but output file was not found in $DIST_DIR!${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}--------------------------------------------------------------------${NC}"
    echo -e "${YELLOW}[NOTE] Inno Setup compiler (ISCC) or Wine was not found in Arch.${NC}"
    echo -e "To compile the installer directly on Arch Linux, install wine & innoextract:"
    echo -e "  ${CYAN}sudo pacman -S wine innoextract${NC}"
    echo -e "Then re-run this script:"
    echo -e "  ${CYAN}./packaging/windows/build_installer_arch.sh${NC}"
    echo -e "${YELLOW}--------------------------------------------------------------------${NC}"
    echo -e "Your portable bundle is ready in: ${CYAN}$BUNDLE_DIR${NC}"
    ls -la "$BUNDLE_DIR"
fi
