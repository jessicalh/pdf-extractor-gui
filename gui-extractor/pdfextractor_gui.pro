QT += core gui widgets pdf network sql
CONFIG += c++17

TARGET = pdfextractor_gui

SOURCES += main.cpp \
    promptquery.cpp \
    queryrunner.cpp

HEADERS += promptquery.h \
    queryrunner.h

# Windows specific
win32 {
    # Link Windows libraries
    LIBS += -lws2_32 -lwinmm -lole32 -loleaut32 -luuid -ladvapi32 -lshell32 -luser32 -lkernel32 -lgdi32

    # Application icon
    RC_ICONS = app_icon.ico

    # Version information
    VERSION = 3.0.0.0
    QMAKE_TARGET_COMPANY = "PDF Extractor"
    QMAKE_TARGET_PRODUCT = "PDF Extractor GUI"
    QMAKE_TARGET_DESCRIPTION = "PDF text extraction and AI analysis tool"
    QMAKE_TARGET_COPYRIGHT = "2024"
}

# Build configuration handling
CONFIG(console_build) {
    # Console build for debugging/Claude
    CONFIG += console
    TARGET = pdfextractor_gui_console
    DESTDIR = test_release
    message("Building console version for debugging")
} else {
    # Default: Windows GUI build (no console)
    CONFIG -= console
    CONFIG += windows
    DESTDIR = release
    DEFINES += QT_NO_DEBUG_OUTPUT
    message("Building Windows GUI version")
}

# Separate build directories for different configs
CONFIG(debug, debug|release) {
    OBJECTS_DIR = build/debug/obj
    MOC_DIR = build/debug/moc
} else {
    OBJECTS_DIR = build/release/obj
    MOC_DIR = build/release/moc
    QMAKE_CXXFLAGS_RELEASE += -O2
}

# Custom make targets for different build configurations
console.target = console
console.commands = @echo Building console version... && \
                   $(QMAKE) CONFIG+=console_build CONFIG+=release && \
                   $(MAKE) -f Makefile.Release && \
                   @echo Console build complete: test_release/pdfextractor_gui_console.exe

windows.target = windows
windows.commands = @echo Building Windows GUI version... && \
                   $(QMAKE) CONFIG+=release CONFIG-=console_build && \
                   $(MAKE) -f Makefile.Release && \
                   @echo Windows build complete: release/pdfextractor_gui.exe

clean-all.target = clean-all
clean-all.commands = @echo Cleaning all build artifacts... && \
                     $(DEL_FILE) -r build test_release release Makefile* && \
                     @echo Clean complete

deploy.target = deploy
deploy.commands = @echo Deploying Qt dependencies... && \
                  cd release && \
                  $$[QT_INSTALL_BINS]/windeployqt.exe pdfextractor_gui.exe && \
                  @echo Deployment complete

deploy-console.target = deploy-console
deploy-console.commands = @echo Deploying Qt dependencies for console build... && \
                          @if not exist test_release mkdir test_release && \
                          xcopy /y release\\*.dll test_release\\ && \
                          xcopy /y /e release\\platforms test_release\\platforms\\ && \
                          xcopy /y /e release\\styles test_release\\styles\\ && \
                          xcopy /y /e release\\imageformats test_release\\imageformats\\ && \
                          xcopy /y /e release\\sqldrivers test_release\\sqldrivers\\ && \
                          @echo Console deployment complete

help.target = help
help.commands = @echo "Available targets:" && \
                @echo "  make console       - Build console version (with debug output)" && \
                @echo "  make windows       - Build Windows GUI version (no console)" && \
                @echo "  make deploy        - Deploy Qt dependencies for Windows build" && \
                @echo "  make deploy-console- Deploy Qt dependencies for console build" && \
                @echo "  make clean-all     - Remove all build artifacts" && \
                @echo "  make               - Default: build release version" && \
                @echo "" && \
                @echo "Direct build commands:" && \
                @echo "  qmake CONFIG+=console_build && make" && \
                @echo "  qmake && make"

QMAKE_EXTRA_TARGETS += console windows clean-all deploy deploy-console help

# Default target info
!build_pass:message("Use 'make help' to see available targets")