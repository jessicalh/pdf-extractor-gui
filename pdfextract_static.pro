QT += core pdf
QT -= gui

CONFIG += c++17 console staticlib static release
CONFIG -= app_bundle shared debug

TARGET = pdfextract

SOURCES += main.cpp

# Force static linking
QMAKE_LFLAGS = -static -static-libgcc -static-libstdc++ -Wl,-Bstatic

# Define static Qt
DEFINES += QT_STATIC_BUILD

# Windows specific
win32 {
    CONFIG += console
    CONFIG -= windows

    # Windows libraries
    LIBS += -lws2_32 -lwinmm -lole32 -loleaut32 -luuid -ladvapi32 -lshell32 -luser32 -lkernel32 -lgdi32
    LIBS += -lmingw32 -lqtpcre2 -lz

    # Link order matters - Qt libs first
    LIBS = $$QMAKE_LIBS_QT_ENTRY $$LIBS
}