# PDF Extractor GUI - Master Build Makefile
# This wraps qmake-generated makefiles with convenient targets

QMAKE = C:/Qt/6.10.0/llvm-mingw_64/bin/qmake.exe
MAKE = mingw32-make
WINDEPLOYQT = C:/Qt/6.10.0/llvm-mingw_64/bin/windeployqt.exe

.PHONY: all console windows clean clean-all deploy deploy-console help

# Default target - build Windows GUI version
all: windows

# Build console version (with debug output)
console:
	@echo "===== Building Console Version ====="
	$(QMAKE) CONFIG+=console_build CONFIG+=release
	$(MAKE) -f Makefile.Release
	@echo "===== Console build complete: test_release/pdfextractor_gui_console.exe ====="

# Build Windows GUI version (no console window)
windows:
	@echo "===== Building Windows GUI Version ====="
	$(QMAKE) CONFIG+=release CONFIG-=console_build
	$(MAKE) -f Makefile.Release
	@echo "===== Windows build complete: release/pdfextractor_gui.exe ====="

# Build both versions
both: console windows

# Clean current configuration
clean:
	-$(MAKE) clean

# Clean everything
clean-all:
	@echo "===== Cleaning all build artifacts ====="
	-cmd /c "rmdir /s /q build 2>nul"
	-cmd /c "rmdir /s /q test_release 2>nul"
	-cmd /c "rmdir /s /q release 2>nul"
	-cmd /c "del /q Makefile Makefile.* 2>nul"
	-cmd /c "del /q .qmake.stash 2>nul"
	@echo "===== Clean complete ====="

# Deploy Qt dependencies for Windows build
deploy: windows
	@echo "===== Deploying Qt dependencies ====="
	cd release && $(WINDEPLOYQT) pdfextractor_gui.exe
	@echo "===== Deployment complete ====="

# Deploy Qt dependencies for console build
deploy-console: console
	@echo "===== Deploying console dependencies ====="
	@if not exist test_release mkdir test_release
	xcopy /y release\*.dll test_release\ >nul 2>&1
	xcopy /y /e release\platforms test_release\platforms\ >nul 2>&1
	xcopy /y /e release\styles test_release\styles\ >nul 2>&1
	xcopy /y /e release\imageformats test_release\imageformats\ >nul 2>&1
	xcopy /y /e release\sqldrivers test_release\sqldrivers\ >nul 2>&1
	@echo "===== Console deployment complete ====="

# Show help
help:
	@echo "PDF Extractor GUI - Build System"
	@echo "================================"
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build Windows GUI version (default)"
	@echo "  make console      - Build console version (with debug output)"
	@echo "  make windows      - Build Windows GUI version (no console)"
	@echo "  make both         - Build both versions"
	@echo "  make deploy       - Build and deploy Windows version with Qt libraries"
	@echo "  make deploy-console - Build and deploy console version with Qt libraries"
	@echo "  make clean        - Clean current build"
	@echo "  make clean-all    - Remove all build artifacts"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Executables:"
	@echo "  release/pdfextractor_gui.exe         - Windows GUI (no console)"
	@echo "  test_release/pdfextractor_gui_console.exe - Console version (debug output)"
	@echo ""
	@echo "Usage examples:"
	@echo "  make              # Build Windows GUI version"
	@echo "  make console      # Build console version for debugging"
	@echo "  make both         # Build both versions"
	@echo "  make clean-all && make both  # Full rebuild of both"