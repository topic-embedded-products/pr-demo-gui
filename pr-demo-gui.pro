#-------------------------------------------------
#
# Project created by QtCreator 2016-01-26T05:32:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pr-demo-gui
TEMPLATE = app

# include dyplo
LIBS        +=  -L$(HOME)/lib -ldyplo
INCLUDEPATH +=  $(HOME)/include

SOURCES +=  main.cpp\
            mainwindow.cpp \
            filterprogrammer.cpp \
            dyplomandelbrotthread.cpp \
            fpgamandelbrotwidget.cpp \
            videowidget.cpp \
            dyplovideothread.cpp



HEADERS  += mainwindow.h \
            filterprogrammer.h \
            dyplomandelbrotthread.h \
            fpgamandelbrotwidget.h \
            videowidget.h \
            dyplovideothread.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
