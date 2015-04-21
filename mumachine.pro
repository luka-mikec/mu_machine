#-------------------------------------------------
#
# Project created by QtCreator 2015-04-20T21:35:12
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mumachine
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11


SOURCES += main.cpp\
        mainwindow.cpp \
    mu_machine.cpp

HEADERS  += mainwindow.h \
    mu_machine.h

FORMS    += mainwindow.ui
