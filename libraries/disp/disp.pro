#--------------------------------------------------------------------------------------------------------------
#
# @file     disp.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     June, 2013
#
# @section  LICENSE
#
# Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that
# the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
#       following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
#       the following disclaimer in the documentation and/or other materials provided with the distribution.
#     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
#       to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# @brief    This project file builds the display library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT  += core widgets svg

# Deep Model Viewer
qtHaveModule(printsupport): QT += printsupport
qtHaveModule(opengl): QT += opengl
qtHaveModule(charts): QT += charts

DEFINES += DISP_LIBRARY

TARGET = Disp
TARGET = $$join(TARGET,,MNE$${MNE_LIB_VERSION},)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Inversed
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse
}

# CNTK related stuff
!isEmpty( CNTK_INCLUDE_DIR ) {
    LIBS += -L$${CNTK_LIBRARY_DIR}
    CONFIG(debug, debug|release) {
        LIBS += -lMNE$${MNE_LIB_VERSION}Deepd \
                -lCntk.Core-2.0
    }
    else {
        LIBS += -lMNE$${MNE_LIB_VERSION}Deep \
                -lCntk.Core-2.0
    }
}



DESTDIR = $${MNE_LIBRARY_DIR}

contains(MNECPP_CONFIG, build_MNECPP_Static_Lib) {
    CONFIG += staticlib
    DEFINES += BUILD_MNECPP_STATIC_LIB
}
else {
    CONFIG += dll

    #
    # win32: copy dll's to bin dir
    # unix: add lib folder to LD_LIBRARY_PATH
    #
    win32 {
        FILE = $${DESTDIR}/$${TARGET}.dll
        BINDIR = $${DESTDIR}/../bin
        FILE ~= s,/,\\,g
        BINDIR ~= s,/,\\,g
        QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${BINDIR}) $$escape_expand(\\n\\t)
    }
}

SOURCES += \
    helpers/colormap.cpp \
    imagesc.cpp \
    plot.cpp \
    graph.cpp \
    rtplot.cpp \
    tfplot.cpp \
    filterwindow.cpp \
    helpers/layoutscene.cpp \
    helpers/averagescene.cpp \
    helpers/averagesceneitem.cpp \
    helpers/filterdatadelegate.cpp \
    helpers/filterdatamodel.cpp \
    helpers/filterplotscene.cpp \
    helpers/selectionscene.cpp \
    helpers/selectionsceneitem.cpp \
    selectionmanagerwindow.cpp \
    helpers/chinfomodel.cpp \
    helpers/mneoperator.cpp \
    helpers/roundededgeswidget.cpp \

HEADERS += \
    disp_global.h \
    helpers/colormap.h \
    imagesc.h \
    plot.h \
    graph.h \
    rtplot.h \
    tfplot.h \
    filterwindow.h \
    selectionmanagerwindow.h \
    helpers/layoutscene.h \
    helpers/averagescene.h \
    helpers/averagesceneitem.h \
    helpers/filterdatadelegate.h \
    helpers/filterdatamodel.h \
    helpers/filterplotscene.h \
    helpers/selectionscene.h \
    helpers/selectionsceneitem.h \
    helpers/chinfomodel.h \
    helpers/mneoperator.h \
    helpers/roundededgeswidget.h \

qtHaveModule(charts) {
    SOURCES += \
        bar.cpp \
        spline.cpp \
        lineplot.cpp \

    HEADERS += \
        bar.h \
        spline.h \
        lineplot.h \
}

# CNTK related stuff
!isEmpty( CNTK_INCLUDE_DIR ) {
    SOURCES += \
        deepmodelviewer/controls.cpp \
        deepmodelviewer/edge.cpp \
        deepmodelviewer/node.cpp \
        deepmodelviewer/view.cpp \
        deepmodelviewer/network.cpp \
        deepmodelviewer/deepviewer.cpp

    HEADERS += \
        deepmodelviewer/controls.h \
        deepmodelviewer/edge.h \
        deepmodelviewer/node.h \
        deepmodelviewer/view.h \
        deepmodelviewer/network.h \
        deepmodelviewer/deepviewer.h

    RESOURCES += \
        deepmodelviewer/images.qrc
}

RESOURCE_FILES +=\
    $${MNE_DIR}/resources/general/default_filters/BP_1Hz_40Hz_Fs1kHz.txt \
    $${MNE_DIR}/resources/general/default_filters/BP_1Hz_70Hz_Fs1kHz.txt \
    $${MNE_DIR}/resources/general/default_filters/filter_default_template.txt \
    $${MNE_DIR}/resources/general/default_filters/NOTCH_50Hz_Fs1kHz.txt \
    $${MNE_DIR}/resources/general/default_filters/NOTCH_60Hz_Fs1kHz.txt \
    $${MNE_DIR}/resources/general/2DLayouts/babymeg-mag-inner-layer.lout \
    $${MNE_DIR}/resources/general/2DLayouts/babymeg-mag-outer-layer.lout \
    $${MNE_DIR}/resources/general/2DLayouts/babymeg-mag-ref.lout \
    $${MNE_DIR}/resources/general/2DLayouts/CTF-275.lout \
    $${MNE_DIR}/resources/general/2DLayouts/magnesWH3600.lout \
    $${MNE_DIR}/resources/general/2DLayouts/standard_waveguard64_duke.lout \
    $${MNE_DIR}/resources/general/2DLayouts/Vectorview-all.lout \
    $${MNE_DIR}/resources/general/2DLayouts/Vectorview-frad.lout \
    $${MNE_DIR}/resources/general/2DLayouts/Vectorview-mag.lout \
    $${MNE_DIR}/resources/general/selectionGroups/mne_browse_raw.sel \
    $${MNE_DIR}/resources/general/selectionGroups/mne_browse_raw_babyMEG.sel \
    $${MNE_DIR}/resources/general/selectionGroups/mne_browse_raw_CTF_275.sel \
    $${MNE_DIR}/resources/general/selectionGroups/mne_browse_raw_Magnes_3600WH.sel \
    $${MNE_DIR}/resources/general/selectionGroups/mne_browse_raw_vv.sel \
    $${MNE_DIR}/resources/general/selectionGroups/mne_browse_raw_vv_new.sel \

# Copy resource files to bin resource folder
for(FILE, RESOURCE_FILES) {
    FILEDIR = $$dirname(FILE)
    FILEDIR ~= s,/resources,/bin/resources,g
    FILEDIR ~= s,/,\\,g
    TRGTDIR = $${FILEDIR}

    QMAKE_POST_LINK += $$sprintf($${QMAKE_MKDIR_CMD}, "$${TRGTDIR}") $$escape_expand(\n\t)

    FILE ~= s,/,\\,g
    TRGTDIR ~= s,/,\\,g
    QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${TRGTDIR}) $$escape_expand(\\n\\t)
}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${CNTK_INCLUDE_DIR}

# Install headers to include directory
header_files.files = ./*.h
header_files.path = $${MNE_INCLUDE_DIR}/disp

INSTALLS += header_files

FORMS += \
    filterwindowwidget.ui \
    selectionmanagerwindow.ui

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

# Deploy Qt Dependencies
win32 {
    isEmpty(TARGET_EXT) {
        TARGET_CUSTOM_EXT = .dll
    } else {
        TARGET_CUSTOM_EXT = $${TARGET_EXT}
    }

    DEPLOY_COMMAND = windeployqt

    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))

    #  # Uncomment the following line to help debug the deploy command when running qmake
    #  warning($${DEPLOY_COMMAND} $${DEPLOY_TARGET})
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET}
}
