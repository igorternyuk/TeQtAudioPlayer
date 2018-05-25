#-------------------------------------------------
#
# Project created by QtCreator 2017-11-02T14:08:35
#
#-------------------------------------------------

QT       += core gui multimedia
CONFIG += c++1z
DEFINES += DEBUG
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TeQtAudioPlayer
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    settingsutil.cpp

HEADERS  += widget.h \
    settingsutil.h

FORMS    += widget.ui

RESOURCES += \
    resources.qrc
