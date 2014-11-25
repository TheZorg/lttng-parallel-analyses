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

TEMPLATE = app


SOURCES += src/main.cpp \
    src/traceanalysis.cpp \
    src/countanalysis.cpp \
    src/tracewrapper.cpp

HEADERS += \
    src/traceanalysis.h \
    src/countanalysis.h \
    src/tracewrapper.h

QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/contrib/babeltrace/lib/.libs\''
QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/contrib/babeltrace/formats/ctf/.libs\''

QMAKE_CXXFLAGS += -fpermissive
QMAKE_CXXFLAGS += -std=gnu++0x
QMAKE_CXXFLAGS += -isystem$$PWD/contrib/babeltrace/include


LIBS += -L$$PWD/contrib/babeltrace/lib/.libs/ -lbabeltrace
LIBS += -L$$PWD/contrib/babeltrace/formats/ctf/.libs/ -lbabeltrace-ctf
LIBS += -lglib-2.0

INCLUDEPATH += /usr/include/glib-2.0/ /usr/lib/x86_64-linux-gnu/glib-2.0/include/
DEPENDPATH += $$PWD/contrib/babeltrace/include

