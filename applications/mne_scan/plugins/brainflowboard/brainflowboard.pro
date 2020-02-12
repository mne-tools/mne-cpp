include(../../../../mne-cpp.pri)

QT += core widgets

TEMPLATE = lib
DEFINES += BRAINFLOWBOARD_LIBRARY

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = $${MNE_BINARY_DIR}/mne_scan_plugins

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lscMeasd \
            -lscDispd \
            -lscSharedd
} else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lscMeas \
            -lscDisp \
            -lscShared
}

contains(QT_ARCH, i386) {
    LIBS += -L"$$PWD/brainflow/installed/lib/" -lBrainflow32  -lDataHandler32 -lBoardController32
} else {
    LIBS += -L"$$PWD/brainflow/installed/lib/" -lBrainflow  -lDataHandler -lBoardController
}

INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
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

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

DISTFILES += \
    brainflowboard.json

OTHER_FILES += \
    brainflowboard.json

FORMS += \
    FormFiles/brainflowsetupwidget.ui \
    FormFiles/brainflowstreamingwidget.ui

# Activate FFTW backend in Eigen for non-static builds only
contains(MNECPP_CONFIG, useFFTW):!contains(MNECPP_CONFIG, static) {
    DEFINES += EIGEN_FFTW_DEFAULT
    INCLUDEPATH += $$shell_path($${FFTW_DIR_INCLUDE})
    LIBS += -L$$shell_path($${FFTW_DIR_LIBS})

    win32 {
        # On Windows
        LIBS += -llibfftw3-3 \
                -llibfftw3f-3 \
                -llibfftw3l-3 \
    }

    unix:!macx {
        # On Linux
        LIBS += -lfftw3 \
                -lfftw3_threads \
    }
}
