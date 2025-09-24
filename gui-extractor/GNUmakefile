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

QMAKE = C:/Qt/6.10.0/llvm-mingw_64/bin/qmake.exe
MAKE = mingw32-make
WINDEPLOYQT = C:/Qt/6.10.0/llvm-mingw_64/bin/windeployqt.exe

.PHONY: all release release-console debug debug-console clean deploy help

# Default target - Release Windows GUI (for end users)
all: release

# Release Windows GUI (no console) - Production build for end users
release:
	@echo "================================================"
	@echo "Building Release Windows GUI (no console)"
	@echo "Target: build/release/pdfextractor_gui.exe"
	@echo "================================================"
	@$(QMAKE) CONFIG+=release CONFIG-=debug CONFIG-=console
	@$(MAKE) -f Makefile.Release
	@echo "Build complete: build/release/pdfextractor_gui.exe"

# Release with Console - For Claude/debugging
release-console:
	@echo "================================================"
	@echo "Building Release with Console (for Claude/debug)"
	@echo "Target: build/release-console/pdfextractor_gui_console.exe"
	@echo "================================================"
	@$(QMAKE) CONFIG+=release CONFIG-=debug CONFIG+=console
	@$(MAKE) -f Makefile.Release
	@echo "Build complete: build/release-console/pdfextractor_gui_console.exe"

# Debug Windows GUI
debug:
	@echo "================================================"
	@echo "Building Debug Windows GUI"
	@echo "Target: build/debug/pdfextractor_gui_debug.exe"
	@echo "================================================"
	@$(QMAKE) CONFIG+=debug CONFIG-=release CONFIG-=console
	@$(MAKE) -f Makefile.Debug
	@echo "Build complete: build/debug/pdfextractor_gui_debug.exe"

# Debug with Console
debug-console:
	@echo "================================================"
	@echo "Building Debug with Console"
	@echo "Target: build/debug-console/pdfextractor_gui_debug_console.exe"
	@echo "================================================"
	@$(QMAKE) CONFIG+=debug CONFIG-=release CONFIG+=console
	@$(MAKE) -f Makefile.Debug
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

# Deploy Qt dependencies for release build
deploy: release
	@echo "Deploying Qt dependencies for release..."
	@cd build/release && $(WINDEPLOYQT) pdfextractor_gui.exe
	@echo "Deployment complete"

# Deploy Qt dependencies for Claude build
deploy-claude: release-console
	@echo "Deploying Qt dependencies for Claude build..."
	@cd build/release-console && $(WINDEPLOYQT) pdfextractor_gui_console.exe
	@echo "Deployment complete"

# Clean all builds
clean:
	@echo "Cleaning all build artifacts..."
	@cmd /c "if exist build rmdir /s /q build 2>nul"
	@cmd /c "if exist Makefile del /q Makefile 2>nul"
	@cmd /c "if exist Makefile.Debug del /q Makefile.Debug 2>nul"
	@cmd /c "if exist Makefile.Release del /q Makefile.Release 2>nul"
	@cmd /c "if exist .qmake.stash del /q .qmake.stash 2>nul"
	@cmd /c "if exist object_script.*.Debug del /q object_script.*.Debug 2>nul"
	@cmd /c "if exist object_script.*.Release del /q object_script.*.Release 2>nul"
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
	@echo "  make release      - Release Windows GUI (no console)"
	@echo "  make release-console - Release with console output"
	@echo "  make debug        - Debug Windows GUI"
	@echo "  make debug-console - Debug with console output"
	@echo "  make all          - Build all 4 configurations"
	@echo ""
	@echo "Deployment:"
	@echo "  make deploy       - Deploy release build with Qt libraries"
	@echo "  make deploy-claude - Deploy Claude build with Qt libraries"
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