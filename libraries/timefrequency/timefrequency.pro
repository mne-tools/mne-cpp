
include(../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT += network concurrent
QT -= gui

DEFINES += TIMEFREQUENCY_LIBRARY

DESTDIR = $${MNE_LIBRARY_DIR}

TARGET = TimeFrequency
TARGET = $$join(TARGET,,mnecpp,)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppUtilsd \
} else {
    LIBS += -lmnecppUtils \
}

SOURCES += \
    convolver.cpp \
    interface.cpp \
    timefrequencyanalyzer.cpp \
    wavelet_spectrum_analyzer.cpp


HEADERS += \
    convolver.h \
    morlet.h \
    timefrequencyanalyzer.h \
    wavelet_spectrum_analyzer.h


INCLUDEPATH += $${MNE_INCLUDE_DIR}

header_files.files = $${HEADERS}
header_files.path = $${MNE_INSTALL_INCLUDE_DIR}/events

INSTALLS += header_files

contains(MNECPP_CONFIG, withCodeCov) {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

win32:!contains(MNECPP_CONFIG, static) {
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($${MNE_LIBRARY_DIR}/$${TARGET}.dll) $${MNE_BINARY_DIR}
}

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
}

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -llibfftw3f-3
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -llibfftw3f-3d
else:unix: LIBS += -L$$PWD/lib/ -llibfftw3f-3

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include
