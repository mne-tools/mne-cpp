include(../../../../mne-cpp.pri)

QT += core widgets

TEMPLATE = lib
DEFINES += BRAINFLOWBOARD_LIBRARY

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = $${MNE_BINARY_DIR}/mne_scan_plugins

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICLIB
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lscMeasd \
            -lscDispd \
            -lscSharedd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lscMeas \
            -lscDisp \
            -lscShared
}

contains(QT_ARCH, i386) {
    LIBS += -L"brainflow/installed/lib/" -lBrainflow32  -lDataHandler32 -lBoardController32
} else {
    LIBS += -L"brainflow/installed/lib/" -lBrainflow  -lDataHandler -lBoardController
}

INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}
INCLUDEPATH += $$PWD/brainflow/installed/inc
DEPENDPATH += $$PWD/brainflow/installed/inc

SOURCES += \
    FormFiles/brainflowsetupwidget.cpp \
    FormFiles/brainflowstreamingwidget.cpp \
    brainflowboard.cpp

HEADERS += \
    FormFiles/brainflowsetupwidget.h \
    FormFiles/brainflowstreamingwidget.h \
    brainflowboard_global.h \
    brainflowboard.h

COPY_CMD = $$copyResources($${RESOURCE_FILES})

QMAKE_POST_LINK += $${COPY_CMD}

UI_DIR = $$PWD

unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    brainflowboard.json

OTHER_FILES += \
    brainflowboard.json

FORMS += \
    FormFiles/brainflowsetupwidget.ui \
    FormFiles/brainflowstreamingwidget.ui
