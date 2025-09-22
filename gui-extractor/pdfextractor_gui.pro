QT += core gui widgets pdf network
CONFIG += c++17

TARGET = pdfextractor_gui

SOURCES += main.cpp
HEADERS += tomlparser.h

# Windows specific - ensure console app for debugging
win32 {
    CONFIG += console

    # Link Windows libraries
    LIBS += -lws2_32 -lwinmm -lole32 -loleaut32 -luuid -ladvapi32 -lshell32 -luser32 -lkernel32 -lgdi32
}