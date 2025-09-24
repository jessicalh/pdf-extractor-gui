#!/bin/bash
# ===================================================
# PDF Extractor GUI - Unix/Mac Build Environment Setup
# ===================================================
#
# This sets up environment variables for the build system.
# Run this before using make if Qt is not in the default location.
#
# Usage: source setup_env.sh
# Then:  make release-console

echo "Setting up PDF Extractor GUI build environment..."

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "Detected macOS"

    # Qt Installation Settings for macOS
    # Modify these for your Qt installation
    export QT_VERSION=6.10.0
    export QT_COMPILER=macos

    # Common Qt installation paths on macOS
    if [ -d "$HOME/Qt/$QT_VERSION/$QT_COMPILER" ]; then
        export QT_BASE="$HOME/Qt/$QT_VERSION/$QT_COMPILER"
    elif [ -d "/usr/local/opt/qt@6" ]; then
        export QT_BASE="/usr/local/opt/qt@6"
    elif [ -d "/opt/homebrew/opt/qt@6" ]; then
        export QT_BASE="/opt/homebrew/opt/qt@6"
    else
        export QT_BASE="$HOME/Qt/$QT_VERSION/$QT_COMPILER"
    fi

    export DEPLOY_TOOL="$QT_BASE/bin/macdeployqt"

else
    # Linux
    echo "Detected Linux"

    # Qt Installation Settings for Linux
    export QT_VERSION=6.10.0
    export QT_COMPILER=gcc_64

    # Common Qt installation paths on Linux
    if [ -d "$HOME/Qt/$QT_VERSION/$QT_COMPILER" ]; then
        export QT_BASE="$HOME/Qt/$QT_VERSION/$QT_COMPILER"
    elif [ -d "/usr/lib/qt6" ]; then
        export QT_BASE="/usr/lib/qt6"
    else
        export QT_BASE="$HOME/Qt/$QT_VERSION/$QT_COMPILER"
    fi

    export DEPLOY_TOOL="echo 'Note: Install linuxdeployqt for automatic deployment'"
fi

# Common settings
export QMAKE="$QT_BASE/bin/qmake"
export MAKE="make"

# Verify Qt installation
if [ ! -f "$QMAKE" ]; then
    echo "WARNING: Qt not found at $QT_BASE"
    echo "Please edit setup_env.sh to set the correct Qt path"
    echo "Or set QT_BASE environment variable manually"
fi

echo ""
echo "Environment configured:"
echo "  Qt Version:  $QT_VERSION"
echo "  Qt Compiler: $QT_COMPILER"
echo "  Qt Base:     $QT_BASE"
echo "  QMake:       $QMAKE"
echo ""
echo "You can now run:"
echo "  make               - Build release GUI"
echo "  make release-console - Build with console for debugging"
echo "  make clean         - Clean all build artifacts"
echo "  make help          - Show all targets"
echo ""