QT += core pdf network
QT -= gui

CONFIG += c++17 console static
CONFIG -= app_bundle shared

TARGET = pdfextract

SOURCES += main_enhanced.cpp
HEADERS += tomlparser.h

# Static linking configuration
QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++
DEFINES += QT_STATIC_BUILD

# Windows specific - ensure console app
win32 {
    CONFIG += console
    CONFIG -= windows

    # Link Windows libraries statically
    LIBS += -lws2_32 -lwinmm -lole32 -loleaut32 -luuid -ladvapi32 -lshell32 -luser32 -lkernel32 -lgdi32 -lcomdlg32
    LIBS += -lshlwapi -lrpcrt4 -lcomctl32

    # Ensure static runtime
    QMAKE_CXXFLAGS_RELEASE += -MT
    QMAKE_CFLAGS_RELEASE += -MT

    # Force static Qt libraries
    QMAKE_LIBS_QT_ENTRY = -lQt6EntryPoint
}