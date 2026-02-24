#!/bin/bash
#
# Prism Build Script
# Builds Linux and Windows VST3 + Standalone targets from a Linux host.
# Windows build is cross-compiled using MinGW.
#
# NOTE: If the source tree is on a network share (NAS/NFS/SMB), the source
# is automatically copied to a local temp directory for building, since CMake
# cannot reliably create files on network filesystems.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

JOBS="$(nproc 2>/dev/null || echo 4)"
BUILD_TYPE="Release"
BUILD_LINUX=true
BUILD_WINDOWS=true

# --- Colors ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

info()  { echo -e "${CYAN}[INFO]${NC}  $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
err()   { echo -e "${RED}[ERROR]${NC} $*"; }

# --- Parse arguments ---
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --linux-only      Build only Linux targets (VST3 + Standalone)"
    echo "  --windows-only    Build only Windows targets (VST3 + Standalone)"
    echo "  --debug           Build in Debug mode instead of Release"
    echo "  --clean           Remove all build directories before building"
    echo "  --jobs N          Number of parallel build jobs (default: $(nproc 2>/dev/null || echo 4))"
    echo "  --skip-deps       Skip dependency installation"
    echo "  -h, --help        Show this help message"
    exit 0
}

SKIP_DEPS=false
CLEAN=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --linux-only)   BUILD_WINDOWS=false; shift ;;
        --windows-only) BUILD_LINUX=false;   shift ;;
        --debug)        BUILD_TYPE="Debug";  shift ;;
        --clean)        CLEAN=true;          shift ;;
        --jobs)         JOBS="$2";           shift 2 ;;
        --skip-deps)    SKIP_DEPS=true;      shift ;;
        -h|--help)      usage ;;
        *) err "Unknown option: $1"; usage ;;
    esac
done

# =============================================================================
# Detect network filesystem and set up local build directory
# =============================================================================

# Check if a path is on a network/remote filesystem
is_network_fs() {
    local dir="$1"
    local fstype
    fstype="$(df -T "$dir" 2>/dev/null | awk 'NR==2 {print $2}')" || true

    case "$fstype" in
        nfs|nfs4|cifs|smb|smbfs|fuse.sshfs|9p|afs)
            return 0 ;;
        *)
            # Also check the mount options for network indicators
            if mount | grep -q "$(df "$dir" 2>/dev/null | awk 'NR==2 {print $1}')" 2>/dev/null; then
                local mount_src
                mount_src="$(df "$dir" 2>/dev/null | awk 'NR==2 {print $1}')"
                # Network mounts typically have :/ or // in the source
                if echo "$mount_src" | grep -qE '(:/|^//)'; then
                    return 0
                fi
            fi
            return 1 ;;
    esac
}

# SOURCE_DIR  = where the original source lives (possibly on NAS)
# WORK_DIR    = where we actually run cmake (always local)
SOURCE_DIR="$SCRIPT_DIR"
WORK_DIR="$SCRIPT_DIR"
LOCAL_COPY=false
LOCAL_BUILD_ROOT="$HOME/.cache/prism-build"

setup_work_dir() {
    if is_network_fs "$SOURCE_DIR"; then
        warn "Source tree is on a network filesystem."
        info "Copying source to local directory for building..."

        LOCAL_COPY=true
        WORK_DIR="$LOCAL_BUILD_ROOT/source"

        # Sync source files to local copy (exclude old build artifacts)
        mkdir -p "$WORK_DIR"
        rsync -a --delete \
            --exclude='build-linux/' \
            --exclude='build-windows/' \
            --exclude='build-host-tools-src/' \
            --exclude='build-win/' \
            --exclude='build/' \
            --exclude='linux-artifacts/' \
            --exclude='windows-artifacts/' \
            --exclude='mac-artifacts/' \
            --exclude='dist/' \
            --exclude='.git/' \
            "$SOURCE_DIR/" "$WORK_DIR/"

        ok "Source copied to $WORK_DIR"
    else
        info "Source tree is on a local filesystem, building in-place."
    fi
}

# Copy final artifacts back to the source directory
copy_artifacts_back() {
    if $LOCAL_COPY; then
        info "Copying build artifacts back to source directory..."
        if [ -d "$WORK_DIR/linux-artifacts" ]; then
            rm -rf "$SOURCE_DIR/linux-artifacts"
            mkdir -p "$SOURCE_DIR/linux-artifacts"
            cp -a "$WORK_DIR/linux-artifacts/." "$SOURCE_DIR/linux-artifacts/"
        fi
        if [ -d "$WORK_DIR/windows-artifacts" ]; then
            rm -rf "$SOURCE_DIR/windows-artifacts"
            mkdir -p "$SOURCE_DIR/windows-artifacts"
            cp -a "$WORK_DIR/windows-artifacts/." "$SOURCE_DIR/windows-artifacts/"
        fi
        # Also copy back host tools so they can be reused next time
        if [ -d "$WORK_DIR/build-host-tools" ]; then
            mkdir -p "$SOURCE_DIR/build-host-tools"
            cp -f "$WORK_DIR"/build-host-tools/* "$SOURCE_DIR/build-host-tools/" 2>/dev/null || true
        fi
        ok "Artifacts copied to source directory."
    fi
}

# =============================================================================
# Dependency check and installation
# =============================================================================

detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "${ID:-unknown}"
    elif command -v lsb_release &>/dev/null; then
        lsb_release -is | tr '[:upper:]' '[:lower:]'
    else
        echo "unknown"
    fi
}

install_deps() {
    local DISTRO
    DISTRO="$(detect_distro)"
    info "Detected distro: $DISTRO"

    case "$DISTRO" in
        ubuntu|debian|linuxmint|pop|raspbian)
            install_deps_apt
            ;;
        fedora)
            install_deps_dnf
            ;;
        arch|manjaro|endeavouros)
            install_deps_pacman
            ;;
        opensuse*|sles)
            install_deps_zypper
            ;;
        *)
            warn "Unsupported distro '$DISTRO'. Please install dependencies manually."
            warn "Required: cmake, g++, git, pkg-config, rsync, and JUCE Linux deps (ALSA, X11, Freetype, GL)"
            if $BUILD_WINDOWS; then
                warn "For Windows cross-compilation: mingw-w64"
            fi
            return 0
            ;;
    esac
}

# --- Debian / Ubuntu / Raspberry Pi OS ---
install_deps_apt() {
    local PKGS=(
        build-essential
        cmake
        git
        pkg-config
        rsync
        # JUCE Linux dependencies
        libasound2-dev
        libx11-dev
        libxrandr-dev
        libxinerama-dev
        libxcursor-dev
        libglu1-mesa-dev
        mesa-common-dev
        libxcomposite-dev
        libxext-dev
    )

    if $BUILD_WINDOWS; then
        PKGS+=( mingw-w64 g++-mingw-w64-x86-64-posix gcc-mingw-w64-x86-64-posix )
    fi

    info "Checking and installing APT packages..."
    local TO_INSTALL=()
    for pkg in "${PKGS[@]}"; do
        if ! dpkg -s "$pkg" &>/dev/null; then
            TO_INSTALL+=("$pkg")
        fi
    done

    # Freetype: package name varies across distro versions
    if ! dpkg -s libfreetype-dev &>/dev/null && ! dpkg -s libfreetype6-dev &>/dev/null; then
        TO_INSTALL+=("libfreetype-dev")
    fi

    if [ ${#TO_INSTALL[@]} -eq 0 ]; then
        ok "All APT dependencies already installed."
    else
        info "Installing: ${TO_INSTALL[*]}"
        sudo apt-get update -qq
        # Try installing; fall back on libfreetype6-dev if libfreetype-dev fails
        if ! sudo apt-get install -y -qq "${TO_INSTALL[@]}" 2>/dev/null; then
            warn "Retrying with libfreetype6-dev fallback..."
            TO_INSTALL=("${TO_INSTALL[@]/libfreetype-dev/libfreetype6-dev}")
            sudo apt-get install -y -qq "${TO_INSTALL[@]}"
        fi
        ok "APT dependencies installed."
    fi
}

# --- Fedora ---
install_deps_dnf() {
    local PKGS=(
        gcc-c++
        cmake
        git
        pkgconfig
        rsync
        alsa-lib-devel
        libX11-devel
        libXrandr-devel
        libXinerama-devel
        libXcursor-devel
        freetype-devel
        mesa-libGLU-devel
        mesa-libGL-devel
        libXcomposite-devel
        libXext-devel
    )

    if $BUILD_WINDOWS; then
        PKGS+=( mingw64-gcc-c++ mingw64-winpthreads-static )
    fi

    info "Installing DNF packages..."
    sudo dnf install -y "${PKGS[@]}"
    ok "DNF dependencies installed."
}

# --- Arch ---
install_deps_pacman() {
    local PKGS=(
        base-devel
        cmake
        git
        pkgconf
        rsync
        alsa-lib
        libx11
        libxrandr
        libxinerama
        libxcursor
        freetype2
        glu
        mesa
        libxcomposite
        libxext
    )

    if $BUILD_WINDOWS; then
        PKGS+=( mingw-w64-gcc )
    fi

    info "Installing Pacman packages..."
    sudo pacman -S --needed --noconfirm "${PKGS[@]}"
    ok "Pacman dependencies installed."
}

# --- openSUSE ---
install_deps_zypper() {
    local PKGS=(
        gcc-c++
        cmake
        git
        pkg-config
        rsync
        alsa-devel
        libX11-devel
        libXrandr-devel
        libXinerama-devel
        libXcursor-devel
        freetype2-devel
        glu-devel
        Mesa-libGL-devel
        libXcomposite-devel
        libXext-devel
    )

    if $BUILD_WINDOWS; then
        PKGS+=( cross-x86_64-w64-mingw32-gcc-c++ )
    fi

    info "Installing Zypper packages..."
    sudo zypper install -y "${PKGS[@]}"
    ok "Zypper dependencies installed."
}

# Verify critical tools are actually available after install
verify_tools() {
    local MISSING=()

    command -v cmake  &>/dev/null || MISSING+=("cmake")
    command -v g++    &>/dev/null || MISSING+=("g++")
    command -v git    &>/dev/null || MISSING+=("git")
    command -v rsync  &>/dev/null || MISSING+=("rsync")
    command -v make   &>/dev/null || command -v ninja &>/dev/null || MISSING+=("make or ninja")

    if $BUILD_WINDOWS; then
        if command -v x86_64-w64-mingw32-g++-posix &>/dev/null; then
            ok "Found MinGW (posix threading): x86_64-w64-mingw32-g++-posix"
        elif command -v x86_64-w64-mingw32-g++ &>/dev/null; then
            warn "Found MinGW but -posix variant not available. Build may fail if threading model is win32."
            warn "Install g++-mingw-w64-x86-64-posix for reliable builds."
        else
            MISSING+=("x86_64-w64-mingw32-g++ (mingw-w64)")
        fi
    fi

    if [ ${#MISSING[@]} -ne 0 ]; then
        err "Missing required tools after install: ${MISSING[*]}"
        err "Please install them manually and re-run."
        exit 1
    fi

    ok "All required tools verified."
}

# =============================================================================
# Build functions
# =============================================================================

clean_builds() {
    info "Cleaning build directories..."
    rm -rf "$WORK_DIR/build-linux" \
           "$WORK_DIR/build-windows" \
           "$WORK_DIR/build-host-tools" \
           "$WORK_DIR/build-host-tools-src" \
           "$WORK_DIR/linux-artifacts" \
           "$WORK_DIR/windows-artifacts" \
           "$WORK_DIR/dist"
    # Also clean local cache if we're on NAS
    if $LOCAL_COPY; then
        rm -rf "$LOCAL_BUILD_ROOT"
    fi
    ok "Clean complete."
}

build_host_tools() {
    # The VST3 helper must be built as a native Linux executable
    # so it can run during the Windows cross-compilation step.
    if [ -f "$WORK_DIR/build-host-tools/juce_vst3_helper" ]; then
        ok "Host tools already built (build-host-tools/juce_vst3_helper)."
        return 0
    fi

    info "Building host tools (juce_vst3_helper)..."
    cmake -S "$WORK_DIR" -B "$WORK_DIR/build-host-tools-src" \
        -DCMAKE_BUILD_TYPE=Release \
        -DJUCE_BUILD_EXTRAS=OFF \
        -DJUCE_BUILD_EXAMPLES=OFF \
        > /dev/null 2>&1

    # Build just the vst3 helper
    cmake --build "$WORK_DIR/build-host-tools-src" --target juce_vst3_helper -j"$JOBS" 2>/dev/null || true

    # Find and copy the helper to the expected location
    mkdir -p "$WORK_DIR/build-host-tools"
    local HELPER
    HELPER="$(find "$WORK_DIR/build-host-tools-src" -name 'juce_vst3_helper' -type f 2>/dev/null | head -1 || true)"
    if [ -n "$HELPER" ] && [ -f "$HELPER" ]; then
        cp "$HELPER" "$WORK_DIR/build-host-tools/juce_vst3_helper"
        chmod +x "$WORK_DIR/build-host-tools/juce_vst3_helper"
        ok "Host tools built successfully."
    else
        warn "Could not build juce_vst3_helper. Windows cross-compilation may still work without it."
    fi
}

build_linux() {
    info "=== Building Linux VST3 + Standalone ==="

    cmake -S "$WORK_DIR" -B "$WORK_DIR/build-linux" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DJUCE_BUILD_EXTRAS=OFF \
        -DJUCE_BUILD_EXAMPLES=OFF

    cmake --build "$WORK_DIR/build-linux" --config "$BUILD_TYPE" \
        --target Prism_VST3 Prism_Standalone -j"$JOBS"

    local ARTEFACTS_ROOT="$WORK_DIR/build-linux/Prism_artefacts/$BUILD_TYPE"
    local VST3_PATH="$ARTEFACTS_ROOT/VST3/Prism.vst3"
    local STANDALONE_DIR="$ARTEFACTS_ROOT/Standalone"

    if [ ! -d "$VST3_PATH" ]; then
        err "Linux VST3 bundle not found after build."
        return 1
    fi

    if [ ! -d "$STANDALONE_DIR" ]; then
        err "Linux standalone output not found after build."
        return 1
    fi

    local OUTPUT_DIR="$WORK_DIR/linux-artifacts"
    rm -rf "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"
    cp -R "$VST3_PATH" "$OUTPUT_DIR/"
    cp -R "$STANDALONE_DIR" "$OUTPUT_DIR/"

    ok "Linux artifacts copied to $OUTPUT_DIR"
}

build_windows() {
    info "=== Building Windows VST3 + Standalone (cross-compile) ==="

    # Ensure host tools are available for cross-compilation
    build_host_tools

    # Detect the correct MinGW compiler (posix variant required for std::mutex)
    local MINGW_CC="x86_64-w64-mingw32-gcc"
    local MINGW_CXX="x86_64-w64-mingw32-g++"

    if command -v x86_64-w64-mingw32-g++-posix &>/dev/null; then
        MINGW_CC="x86_64-w64-mingw32-gcc-posix"
        MINGW_CXX="x86_64-w64-mingw32-g++-posix"
        ok "Using MinGW posix threading: $MINGW_CXX"
    else
        warn "MinGW -posix variant not found, using default: $MINGW_CXX"
        # Check if the default is already posix
        local THREADING
        THREADING="$($MINGW_CXX -v 2>&1 | grep 'Thread model' || true)"
        info "MinGW thread model: $THREADING"
    fi

    # Always start fresh -- toolchain/compiler changes require full reconfigure,
    # and stale build dirs cause false "success" from leftover artifacts
    if [ -d "$WORK_DIR/build-windows" ]; then
        info "Removing old Windows build directory..."
        rm -rf "$WORK_DIR/build-windows"
    fi

    cmake -S "$WORK_DIR" -B "$WORK_DIR/build-windows" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_TOOLCHAIN_FILE="$WORK_DIR/cmake/windows-toolchain.cmake" \
        -DMINGW_C_COMPILER="$MINGW_CC" \
        -DMINGW_CXX_COMPILER="$MINGW_CXX" \
        -DJUCE_BUILD_EXTRAS=OFF \
        -DJUCE_BUILD_EXAMPLES=OFF

    cmake --build "$WORK_DIR/build-windows" --config "$BUILD_TYPE" \
        --target Prism_VST3 Prism_Standalone -j"$JOBS"

    local ARTEFACTS_ROOT="$WORK_DIR/build-windows/Prism_artefacts/$BUILD_TYPE"
    local VST3_PATH="$ARTEFACTS_ROOT/VST3/Prism.vst3"
    local STANDALONE_DIR="$ARTEFACTS_ROOT/Standalone"

    if [ ! -d "$VST3_PATH" ]; then
        err "Windows VST3 bundle not found after build."
        return 1
    fi

    if [ ! -d "$STANDALONE_DIR" ]; then
        err "Windows standalone output not found after build."
        return 1
    fi

    local OUTPUT_DIR="$WORK_DIR/windows-artifacts"
    rm -rf "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"
    cp -R "$VST3_PATH" "$OUTPUT_DIR/"
    cp -R "$STANDALONE_DIR" "$OUTPUT_DIR/"

    ok "Windows artifacts copied to $OUTPUT_DIR"
}

# =============================================================================
# Main
# =============================================================================

echo ""
echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║      Prism Linux/Windows Build Script    ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════╝${NC}"
echo ""
info "Build type: $BUILD_TYPE"
info "Parallel jobs: $JOBS"
info "Targets: $( $BUILD_LINUX && echo -n 'Linux ' )$( $BUILD_WINDOWS && echo -n 'Windows' )"
echo ""

# Install dependencies first (before we set up work dir, since we need rsync)
if ! $SKIP_DEPS; then
    install_deps
fi

# Verify required tools exist
verify_tools

# Set up local build directory if source is on a network filesystem
setup_work_dir

# Clean if requested (after work dir is set up so we clean the right place)
if $CLEAN; then
    clean_builds
fi

# Build targets
FAILED=()
SUCCEEDED=()

if $BUILD_LINUX; then
    if build_linux; then
        SUCCEEDED+=("Linux")
    else
        FAILED+=("Linux")
    fi
fi

if $BUILD_WINDOWS; then
    if build_windows; then
        SUCCEEDED+=("Windows")
    else
        FAILED+=("Windows")
    fi
fi

# Copy artifacts back to source dir if we built in a local copy
copy_artifacts_back

# Summary
echo ""
echo -e "${CYAN}══════════════════════════════════════════${NC}"
echo -e "${CYAN}  Build Summary${NC}"
echo -e "${CYAN}══════════════════════════════════════════${NC}"

if [ ${#SUCCEEDED[@]} -gt 0 ]; then
    ok "Succeeded: ${SUCCEEDED[*]}"
fi
if [ ${#FAILED[@]} -gt 0 ]; then
    err "Failed:    ${FAILED[*]}"
fi

if [ -d "$SOURCE_DIR/linux-artifacts" ] || [ -d "$SOURCE_DIR/windows-artifacts" ]; then
    echo ""
    info "Output directories:"
    [ -d "$SOURCE_DIR/linux-artifacts" ] && echo "  - $SOURCE_DIR/linux-artifacts"
    [ -d "$SOURCE_DIR/windows-artifacts" ] && echo "  - $SOURCE_DIR/windows-artifacts"
fi

if [ ${#FAILED[@]} -gt 0 ]; then
    exit 1
fi

echo ""
ok "All builds completed successfully!"
