#-------------------------------------------------
#
# Project created by QtCreator 2016-01-26T05:32:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pr-demo-gui
TEMPLATE = app

CONFIG += link_pkgconfig
PKGCONFIG += fftw3f dyplo alsa

SOURCES +=  main.cpp\
            mainwindow.cpp \
            mousemonitor.cpp \
            filterprogrammer.cpp \
            dyplomandelbrotthread.cpp \
            fpgamandelbrotwidget.cpp \
            videowidget.cpp \
            dyplovideothread.cpp \
            dyplocontext.cpp \
            dyplorouter.cpp \
            fourierfilter.cpp \
            spectrumwidget.cpp \
            microphonecapturethread.cpp



HEADERS  += mainwindow.h \
            mousemonitor.h \
            filterprogrammer.h \
            dyplomandelbrotthread.h \
            fpgamandelbrotwidget.h \
            videowidget.h \
            dyplovideothread.h \
            dyplocontext.h \
            dyplorouter.h \
            types.h \
            fourierfilter.h \
            spectrumwidget.h \
            microphonecapturethread.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

target.path = /usr/bin
INSTALLS += target
