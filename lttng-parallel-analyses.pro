#-------------------------------------------------
#
# Project created by QtCreator 2014-11-24T13:45:53
#
#-------------------------------------------------

QT       += core concurrent

QT       -= gui

TARGET = lttng-parallel-analyses
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += link_pkgconfig
CONFIG += debug

TEMPLATE = app


SOURCES += src/main.cpp \
    src/traceanalysis.cpp \
    src/countanalysis.cpp \
    src/tracewrapper.cpp \
    src/cpuanalysis.cpp

HEADERS += \
    src/traceanalysis.h \
    src/countanalysis.h \
    src/tracewrapper.h \
    src/cpuanalysis.h

QMAKE_LFLAGS += '-Wl,-rpath,\'$$PWD/contrib/babeltrace/lib/.libs\''
QMAKE_LFLAGS += '-Wl,-rpath,\'$$PWD/contrib/babeltrace/formats/ctf/.libs\''
QMAKE_LFLAGS += '-Wl,-rpath,\'$$PWD/contrib/tigerbeetle/src/\''

# BUG in ld : https://sourceware.org/bugzilla/show_bug.cgi?id=16936
# Basically, the $ORIGIN in the shared library does not get parsed by the static linker
# That's why we need to use --rpath-link for libdelorean
QMAKE_LFLAGS += '-Wl,-rpath-link,\'$$PWD/contrib/tigerbeetle/contrib/libdelorean/src\''

QMAKE_CXXFLAGS += -fpermissive
QMAKE_CXXFLAGS += -std=gnu++0x

LIBS += -L$$PWD/contrib/babeltrace/lib/.libs/ -lbabeltrace
LIBS += -L$$PWD/contrib/babeltrace/formats/ctf/.libs/ -lbabeltrace-ctf
LIBS += -L$$PWD/contrib/tigerbeetle/src/ -ltigerbeetle
LIBS += -lboost_filesystem -lboost_system

PKGCONFIG += glib-2.0

QMAKE_CXXFLAGS += -isystem$$PWD/contrib/babeltrace/include
DEPENDPATH += $$PWD/contrib/babeltrace/include

QMAKE_CXXFLAGS += -isystem$$PWD/contrib/tigerbeetle/src
INCLUDEPATH += $$PWD/contrib/tigerbeetle/src
DEPENDPATH += $$PWD/contrib/tigerbeetle/src

