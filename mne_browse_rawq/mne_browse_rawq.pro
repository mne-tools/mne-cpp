#-------------------------------------------------
#
# Project created by QtCreator 2012-12-04T18:11:36
#
#-------------------------------------------------

QT  += core gui
QT  += widgets
QT += printsupport

TARGET = mne_browse_rawq
TEMPLATE = app


SOURCES += main.cpp \
        mnebrowserawq.cpp \
        3rdParty/QCustomPlot/qcustomplot.cpp

HEADERS  += \
        mnebrowserawq.h \
        3rdParty/QCustomPlot/qcustomplot.h

FORMS    += mnebrowserawq.ui
