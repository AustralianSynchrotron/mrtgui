#-------------------------------------------------
#
# Project created by QtCreator 2011-10-24T12:58:41
#
#-------------------------------------------------

QT       += core gui

TARGET = mrtgui
TEMPLATE = app


SOURCES += main.cpp\
        mrtwindow.cpp \
    axis.cpp

HEADERS  += mrtwindow.h \
    axis.h

FORMS    += mrtwindow.ui \
    axis.ui



LIBS     += -lqcamotorgui \
            -lmrtShutter -lshutter1A -lcomponent -lcomponentGui

target.files = $$[TARGET]
target.path = $$INSTALLBASE/lib/imbl
INSTALLS += target



