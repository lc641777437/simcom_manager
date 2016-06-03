#-------------------------------------------------
#
# Project created by QtCreator 2016-04-14T17:22:24
#
#-------------------------------------------------

QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SimcomManager
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    eventdialog.cpp \
    finddialog.cpp

HEADERS  += mainwindow.h \
    protocol_manager.h \
    eventdialog.h \
    finddialog.h

FORMS    += mainwindow.ui \
    eventdialog.ui \
    finddialog.ui
