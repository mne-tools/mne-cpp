include(../../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += plugin

DEFINES += FTBUFFER_LIBRARY

QT += widgets
QT += network

TARGET = ftbuffer
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

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

SOURCES += \
    FormFiles/ftbuffergui.cpp \
    ftbuffer.cpp

HEADERS += \
    FormFiles/ftbuffergui.h \
    ftbuffer_global.h \
    ftbuffer.h


COPY_CMD = $$copyResources($${RESOURCE_FILES})
QMAKE_POST_LINK += $${COPY_CMD}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

OTHER_FILES += ftbuffer.json

UI_DIR = $$PWD

unix:!macx {
    # === Unix ===
    QMAKE_RPATHDIR += $ORIGIN/../../lib
}

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $${PWD}

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

# suppress visibility warnings
unix: QMAKE_CXXFLAGS += -Wno-attributes

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

FORMS += \
    FormFiles/ftbuffergui.ui
