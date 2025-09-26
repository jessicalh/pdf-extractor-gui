# PDF Extractor GUI - Qt Project File
# Supports 4 build configurations:
#   - Release (Windows GUI, no console)
#   - Release+Console (with console for debugging)
#   - Debug (Windows GUI with debug symbols)
#   - Debug+Console (debug with console output)

QT += core gui widgets pdf network sql
CONFIG += c++17

TARGET = pdfextractor_gui

SOURCES += main.cpp \
    promptquery.cpp \
    queryrunner.cpp \
    modellistfetcher.cpp \
    inputmethod.cpp \
    zoteroinput.cpp \
    safepdfloader.cpp

HEADERS += promptquery.h \
    queryrunner.h \
    modellistfetcher.h \
    inputmethod.h \
    zoteroinput.h \
    safepdfloader.h

# Windows specific settings
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

# Configuration-specific settings
CONFIG(debug, debug|release) {
    # Debug builds
    CONFIG(console) {
        # Debug + Console
        TARGET = pdfextractor_gui_debug_console
        DESTDIR = build/debug-console
        CONFIG += console
        DEFINES += DEBUG_BUILD CONSOLE_BUILD
        message("Building: Debug with Console")
    } else {
        # Debug only (Windows GUI)
        TARGET = pdfextractor_gui_debug
        DESTDIR = build/debug
        CONFIG -= console
        CONFIG += windows
        DEFINES += DEBUG_BUILD
        message("Building: Debug Windows GUI")
    }
    # Debug-specific settings
    OBJECTS_DIR = $$DESTDIR/obj
    MOC_DIR = $$DESTDIR/moc
    RCC_DIR = $$DESTDIR/rcc
    UI_DIR = $$DESTDIR/ui
} else {
    # Release builds
    CONFIG(console) {
        # Release + Console (for Claude/debugging)
        TARGET = pdfextractor_gui_console
        DESTDIR = build/release-console
        CONFIG += console
        DEFINES += CONSOLE_BUILD
        message("Building: Release with Console")
    } else {
        # Release only (Windows GUI - production)
        TARGET = pdfextractor_gui
        DESTDIR = build/release
        CONFIG -= console
        CONFIG += windows
        DEFINES += QT_NO_DEBUG_OUTPUT
        message("Building: Release Windows GUI")
    }
    # Release-specific settings
    OBJECTS_DIR = $$DESTDIR/obj
    MOC_DIR = $$DESTDIR/moc
    RCC_DIR = $$DESTDIR/rcc
    UI_DIR = $$DESTDIR/ui
    QMAKE_CXXFLAGS_RELEASE += -O2
}

# Build info
!build_pass {
    message("PDF Extractor GUI - Build Configuration")
    message("========================================")
    message("Use 'make help' to see all build targets")
    message("Default build: Release Windows GUI (no console)")
}