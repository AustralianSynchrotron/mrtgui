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

IMBLEXEC   = $$system(command -v imblgui)
IMBLORIGIN = $$dirname(IMBLEXEC)
IMBLRPATH = $$system(objdump -x $${IMBLEXEC} \
                     | grep RPATH \
                     | sed -e \"s/RPATH//g\" \
                           -e \"s/^ *//g\" \
                           -e \"s \\\$ORIGIN $${IMBLORIGIN} g\" \
                     | cut -d':' -f1 )

QMAKE_LFLAGS += -Wl,-rpath,$$IMBLRPATH

LIBS     += -lqcamotorgui \
            -L$$IMBLRPATH \
            -lmrtShutterGui -lshutter1A -lcomponent -lcomponentGui

target.files = $$[TARGET]
target.path = $$INSTALLBASE/bin
INSTALLS += target



