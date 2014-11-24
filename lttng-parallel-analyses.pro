#-------------------------------------------------
#
# Project created by QtCreator 2014-11-24T13:45:53
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = lttng-parallel-analyses
CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += src/main.cpp \
    src/traceanalysis.cpp \
    src/countanalysis.cpp

HEADERS += \
    src/traceanalysis.h \
    src/countanalysis.h
