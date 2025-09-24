# PDF Extractor GUI - Master Build System
# ========================================
# Build configurations:
#   - release         : Windows GUI (no console) - for end users
#   - release-console : With console output - for Claude/debugging
#   - debug          : Windows GUI with debug symbols
#   - debug-console  : Debug with console output
#
# Usage:
#   make              : Build release (default for users)
#   make release-console : Build for Claude (with console)
#   make all          : Build all 4 configurations
#   make clean        : Clean all builds
#   make help         : Show this help

# Qt Configuration - can be overridden by environment variables
QT_VERSION ?= 6.10.0

# Platform detection and defaults
# Check if we're in WSL (Windows Subsystem for Linux)
UNAME_R := $(shell uname -r)
ifneq ($(findstring microsoft,$(UNAME_R)),)
    # WSL - Building Windows binaries from Linux environment
    QT_COMPILER ?= llvm-mingw_64
    QT_BASE ?= /mnt/c/Qt/$(QT_VERSION)/$(QT_COMPILER)
    # Call Windows executables directly - WSL can execute .exe files
    QMAKE ?= $(QT_BASE)/bin/qmake.exe
    MAKE ?= mingw32-make
    DEPLOY_TOOL ?= $(QT_BASE)/bin/windeployqt.exe
    DEPLOY_FLAGS ?= --no-translations
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CD = cd
    AND = &&
    DEVNULL = 2>/dev/null
else ifeq ($(OS),Windows_NT)
    # Native Windows (cmd/PowerShell)
    QT_COMPILER ?= llvm-mingw_64
    QT_BASE ?= C:/Qt/$(QT_VERSION)/$(QT_COMPILER)
    QMAKE ?= $(QT_BASE)/bin/qmake.exe
    MAKE ?= mingw32-make
    DEPLOY_TOOL ?= $(QT_BASE)/bin/windeployqt.exe
    DEPLOY_FLAGS ?= --no-translations
    RM = cmd /c del /q
    RMDIR = cmd /c rmdir /s /q
    MKDIR = cmd /c mkdir
    CD = cd
    AND = &&
    DEVNULL = 2>nul
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        # macOS defaults
        QT_COMPILER ?= macos
        QT_BASE ?= ~/Qt/$(QT_VERSION)/$(QT_COMPILER)
        QMAKE ?= $(QT_BASE)/bin/qmake
        MAKE ?= make
        DEPLOY_TOOL ?= $(QT_BASE)/bin/macdeployqt
        DEPLOY_FLAGS ?=
    else
        # Linux defaults
        QT_COMPILER ?= gcc_64
        QT_BASE ?= ~/Qt/$(QT_VERSION)/$(QT_COMPILER)
        QMAKE ?= $(QT_BASE)/bin/qmake
        MAKE ?= make
        DEPLOY_TOOL ?= echo "Note: Deploy manually or install linuxdeployqt"
        DEPLOY_FLAGS ?=
    endif
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CD = cd
    AND = &&
    DEVNULL = 2>/dev/null
endif

.PHONY: all release release-console debug debug-console clean deploy help

# Default target - Release Windows GUI (for end users)
all: release

# Release Windows GUI (no console) - Production build for end users
release:
	@echo "================================================"
	@echo "Building Release Windows GUI (no console)"
	@echo "Target: build/release/pdfextractor_gui.exe"
	@echo "================================================"
ifeq ($(findstring microsoft,$(UNAME_R)),)
	@$(QMAKE) CONFIG+=release CONFIG-=debug CONFIG-=console
	@$(MAKE) -f Makefile.Release
else
	@cmd.exe /c "C:\Qt\6.10.0\llvm-mingw_64\bin\qmake.exe CONFIG+=release CONFIG-=debug CONFIG-=console && mingw32-make -f Makefile.Release"
endif
	@echo "Deploying Qt dependencies..."
	@cd build/release && $(DEPLOY_TOOL) $(DEPLOY_FLAGS) pdfextractor_gui.exe
	@echo "Build complete: build/release/pdfextractor_gui.exe"

# Release with Console - For Claude/debugging
release-console:
	@echo "================================================"
	@echo "Building Release with Console (for Claude/debug)"
	@echo "Target: build/release-console/pdfextractor_gui_console.exe"
	@echo "================================================"
ifeq ($(findstring microsoft,$(UNAME_R)),)
	@$(QMAKE) CONFIG+=release CONFIG-=debug CONFIG+=console
	@$(MAKE) -f Makefile.Release
else
	@cmd.exe /c "C:\Qt\6.10.0\llvm-mingw_64\bin\qmake.exe CONFIG+=release CONFIG-=debug CONFIG+=console && mingw32-make -f Makefile.Release"
endif
	@echo "Deploying Qt dependencies..."
	@cd build/release-console && $(DEPLOY_TOOL) $(DEPLOY_FLAGS) pdfextractor_gui_console.exe
	@echo "Build complete: build/release-console/pdfextractor_gui_console.exe"

# Debug Windows GUI
debug:
	@echo "================================================"
	@echo "Building Debug Windows GUI"
	@echo "Target: build/debug/pdfextractor_gui_debug.exe"
	@echo "================================================"
ifeq ($(findstring microsoft,$(UNAME_R)),)
	@$(QMAKE) CONFIG+=debug CONFIG-=release CONFIG-=console
	@$(MAKE) -f Makefile.Debug
else
	@cmd.exe /c "C:\Qt\6.10.0\llvm-mingw_64\bin\qmake.exe CONFIG+=debug CONFIG-=release CONFIG-=console && mingw32-make -f Makefile.Debug"
endif
	@echo "Deploying Qt dependencies..."
	@cd build/debug && $(DEPLOY_TOOL) $(DEPLOY_FLAGS) pdfextractor_gui_debug.exe
	@echo "Build complete: build/debug/pdfextractor_gui_debug.exe"

# Debug with Console
debug-console:
	@echo "================================================"
	@echo "Building Debug with Console"
	@echo "Target: build/debug-console/pdfextractor_gui_debug_console.exe"
	@echo "================================================"
ifeq ($(findstring microsoft,$(UNAME_R)),)
	@$(QMAKE) CONFIG+=debug CONFIG-=release CONFIG+=console
	@$(MAKE) -f Makefile.Debug
else
	@cmd.exe /c "C:\Qt\6.10.0\llvm-mingw_64\bin\qmake.exe CONFIG+=debug CONFIG-=release CONFIG+=console && mingw32-make -f Makefile.Debug"
endif
	@echo "Deploying Qt dependencies..."
	@cd build/debug-console && $(DEPLOY_TOOL) $(DEPLOY_FLAGS) pdfextractor_gui_debug_console.exe
	@echo "Build complete: build/debug-console/pdfextractor_gui_debug_console.exe"

# Build all configurations
all: release release-console debug debug-console
	@echo "================================================"
	@echo "All builds complete!"
	@echo "Release: build/release/pdfextractor_gui.exe"
	@echo "Release+Console: build/release-console/pdfextractor_gui_console.exe"
	@echo "Debug: build/debug/pdfextractor_gui_debug.exe"
	@echo "Debug+Console: build/debug-console/pdfextractor_gui_debug_console.exe"
	@echo "================================================"


# Clean all builds
clean:
	@echo "Cleaning all build artifacts..."
	@$(RMDIR) build $(DEVNULL) || true
	@$(RM) Makefile $(DEVNULL) || true
	@$(RM) Makefile.Debug $(DEVNULL) || true
	@$(RM) Makefile.Release $(DEVNULL) || true
	@$(RM) .qmake.stash $(DEVNULL) || true
	@$(RM) object_script.*.Debug $(DEVNULL) || true
	@$(RM) object_script.*.Release $(DEVNULL) || true
	@echo "Clean complete"

# Show help
help:
	@echo "PDF Extractor GUI - Build System"
	@echo "================================"
	@echo ""
	@echo "Quick Commands:"
	@echo "  make              - Build release for Windows users (no console)"
	@echo "  make release-console - Build for Claude/debugging (with console)"
	@echo ""
	@echo "All Build Targets:"
	@echo "  make release      - Release Windows GUI (no console, auto-deploys DLLs)"
	@echo "  make release-console - Release with console output (auto-deploys DLLs)"
	@echo "  make debug        - Debug Windows GUI (auto-deploys DLLs)"
	@echo "  make debug-console - Debug with console output (auto-deploys DLLs)"
	@echo "  make all          - Build all 4 configurations"
	@echo ""
	@echo "Maintenance:"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Output Directories:"
	@echo "  build/release/        - Release Windows GUI"
	@echo "  build/release-console/ - Release with console (Claude)"
	@echo "  build/debug/          - Debug Windows GUI"
	@echo "  build/debug-console/   - Debug with console"
	@echo ""
	@echo "IMPORTANT FOR CLAUDE:"
	@echo "  Always use: make release-console"
	@echo "  This gives you console output for debugging"