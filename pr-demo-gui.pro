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
PKGCONFIG += dyplo

SOURCES +=  main.cpp\
            mainwindow.cpp \
            mousemonitor.cpp \
            videowidget.cpp \
    qprregionlabel.cpp \
            dyplocontext.cpp \
    video-capture.cpp \
    frameratecounter.cpp \
    videopipeline.cpp \
    externalresources.cpp \
    mandelbrotpipeline.cpp \
    colormap.cpp \
    cpu/cpuinfo.cpp

HEADERS  += mainwindow.h \
            mousemonitor.h \
            videowidget.h \
            dyplocontext.h \
            types.h \
    qprregionlabel.h \
    video-capture.h \
    frameratecounter.h \
    videopipeline.h \
    dyploresources.h \
    externalresources.h \
    mandelbrotpipeline.h \
    colormap.h \
    cpu/cpuinfo.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

target.path = /usr/bin
INSTALLS += target
