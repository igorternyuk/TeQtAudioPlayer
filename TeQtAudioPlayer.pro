#-------------------------------------------------
#
# Project created by QtCreator 2017-11-02T14:08:35
#
#-------------------------------------------------

QT       += core gui multimedia
CONFIG += c++1z

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TeQtAudioPlayer
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp

HEADERS  += widget.h

FORMS    += widget.ui

RESOURCES += \
    resources.qrc
