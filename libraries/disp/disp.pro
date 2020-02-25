#==============================================================================================================
#
# @file     disp.pro
# @author   Lorenz Esch <lesch@mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
# @version  dev
# @date     June, 2013
#
# @section  LICENSE
#
# Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
#==============================================================================================================

include(../../mne-cpp.pri)

TEMPLATE = lib

QT += core widgets svg concurrent

qtHaveModule(printsupport): QT += printsupport
qtHaveModule(charts): QT += charts

DEFINES += DISP_LIBRARY

TARGET = Disp
TARGET = $$join(TARGET,,MNE$${MNE_LIB_VERSION},)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, dispOpenGL) {
    qtHaveModule(opengl): QT += opengl
    DEFINES += USE_OPENGL
}

DESTDIR = $${MNE_LIBRARY_DIR}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICLIB
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Inversed
} else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse
}

SOURCES += \
    plots/imagesc.cpp \
    plots/plot.cpp \
    plots/graph.cpp \
    plots/tfplot.cpp \
    plots/helpers/colormap.cpp \
    viewers/filterdesignview.cpp \
    viewers/averagelayoutview.cpp \
    viewers/spectrumview.cpp \
    viewers/modalityselectionview.cpp \
    viewers/butterflyview.cpp \
    viewers/channelselectionview.cpp \
    viewers/spectrumsettingsview.cpp \
    viewers/scalingview.cpp \
    viewers/projectorsview.cpp \
    viewers/compensatorview.cpp \
    viewers/filtersettingsview.cpp \
    viewers/spharasettingsview.cpp \
    viewers/fiffrawviewsettings.cpp \
    viewers/averageselectionview.cpp \
    viewers/triggerdetectionview.cpp \
    viewers/quickcontrolview.cpp \
    viewers/connectivitysettingsview.cpp \
    viewers/minimumnormsettingsview.cpp \
    viewers/averagingsettingsview.cpp \
    viewers/projectsettingsview.cpp \
    viewers/control3dview.cpp \
    viewers/tfsettingsview.cpp \
    viewers/artifactsettingsview.cpp \
    viewers/rtfiffrawview.cpp \
    viewers/helpers/rtfiffrawviewmodel.cpp \
    viewers/helpers/rtfiffrawviewdelegate.cpp \
    viewers/helpers/evokedsetmodel.cpp \
    viewers/helpers/layoutscene.cpp \
    viewers/helpers/averagescene.cpp \
    viewers/helpers/averagesceneitem.cpp \
    viewers/helpers/filterplotscene.cpp \
    viewers/helpers/selectionscene.cpp \
    viewers/helpers/selectionsceneitem.cpp \
    viewers/helpers/channelinfomodel.cpp \
    viewers/helpers/mneoperator.cpp \
    viewers/helpers/draggableframelesswidget.cpp \
    viewers/helpers/frequencyspectrumdelegate.cpp \
    viewers/helpers/frequencyspectrummodel.cpp \

HEADERS += \
    disp_global.h \
    plots/imagesc.h \
    plots/plot.h \
    plots/graph.h \
    plots/tfplot.h \
    plots/helpers/colormap.h \
    viewers/filterdesignview.h \
    viewers/averagelayoutview.h \
    viewers/spectrumview.h \
    viewers/modalityselectionview.h \
    viewers/butterflyview.h \
    viewers/channelselectionview.h \
    viewers/spectrumsettingsview.h \
    viewers/scalingview.h \
    viewers/projectorsview.h \
    viewers/compensatorview.h \
    viewers/filtersettingsview.h \
    viewers/spharasettingsview.h \
    viewers/fiffrawviewsettings.h \
    viewers/averageselectionview.h \
    viewers/triggerdetectionview.h \
    viewers/quickcontrolview.h \
    viewers/connectivitysettingsview.h \
    viewers/minimumnormsettingsview.h \
    viewers/averagingsettingsview.h \
    viewers/projectsettingsview.h \
    viewers/control3dview.h \
    viewers/tfsettingsview.h \
    viewers/artifactsettingsview.h \
    viewers/rtfiffrawview.h \
    viewers/helpers/rtfiffrawviewdelegate.h \
    viewers/helpers/rtfiffrawviewmodel.h \
    viewers/helpers/evokedsetmodel.h \
    viewers/helpers/layoutscene.h \
    viewers/helpers/averagescene.h \
    viewers/helpers/averagesceneitem.h \
    viewers/helpers/filterplotscene.h \
    viewers/helpers/selectionscene.h \
    viewers/helpers/selectionsceneitem.h \
    viewers/helpers/channelinfomodel.h \
    viewers/helpers/mneoperator.h \
    viewers/helpers/draggableframelesswidget.h \
    viewers/helpers/frequencyspectrumdelegate.h \
    viewers/helpers/frequencyspectrummodel.h \

qtHaveModule(charts) {
    SOURCES += \
        plots/bar.cpp \
        plots/spline.cpp \
        plots/lineplot.cpp \

    HEADERS += \
        plots/bar.h \
        plots/spline.h \
        plots/lineplot.h \
}

FORMS += \
    viewers/formfiles/filterdesignview.ui \
    viewers/formfiles/channelselectionview.ui \
    viewers/formfiles/spharasettingsview.ui \
    viewers/formfiles/fiffrawviewsettings.ui \
    viewers/formfiles/triggerdetectionview.ui \
    viewers/formfiles/quickcontrolview.ui \
    viewers/formfiles/tfsettingsview.ui \
    viewers/formfiles/connectivitysettingsview.ui \
    viewers/formfiles/minimumnormsettingsview.ui \
    viewers/formfiles/averagingsettingsview.ui \
    viewers/formfiles/projectsettingsview.ui \
    viewers/formfiles/control3dview.ui \

RESOURCE_FILES +=\
    $${ROOT_DIR}/resources/general/default_filters/BP_1Hz_40Hz_Fs1kHz.txt \
    $${ROOT_DIR}/resources/general/default_filters/BP_1Hz_70Hz_Fs1kHz.txt \
    $${ROOT_DIR}/resources/general/default_filters/filter_default_template.txt \
    $${ROOT_DIR}/resources/general/default_filters/NOTCH_50Hz_Fs1kHz.txt \
    $${ROOT_DIR}/resources/general/default_filters/NOTCH_60Hz_Fs1kHz.txt \
    $${ROOT_DIR}/resources/general/2DLayouts/babymeg-mag-inner-layer.lout \
    $${ROOT_DIR}/resources/general/2DLayouts/babymeg-mag-outer-layer.lout \
    $${ROOT_DIR}/resources/general/2DLayouts/babymeg-mag-ref.lout \
    $${ROOT_DIR}/resources/general/2DLayouts/CTF-275.lout \
    $${ROOT_DIR}/resources/general/2DLayouts/magnesWH3600.lout \
    $${ROOT_DIR}/resources/general/2DLayouts/standard_waveguard64_duke.lout \
    $${ROOT_DIR}/resources/general/2DLayouts/Vectorview-all.lout \
    $${ROOT_DIR}/resources/general/2DLayouts/Vectorview-grad.lout \
    $${ROOT_DIR}/resources/general/2DLayouts/Vectorview-mag.lout \
    $${ROOT_DIR}/resources/general/selectionGroups/mne_browse_raw.sel \
    $${ROOT_DIR}/resources/general/selectionGroups/mne_browse_raw_babyMEG.sel \
    $${ROOT_DIR}/resources/general/selectionGroups/mne_browse_raw_CTF_275.sel \
    $${ROOT_DIR}/resources/general/selectionGroups/mne_browse_raw_Magnes_3600WH.sel \
    $${ROOT_DIR}/resources/general/selectionGroups/mne_browse_raw_vv.sel \
    $${ROOT_DIR}/resources/general/selectionGroups/mne_browse_raw_vv_new.sel \

# Copy resource files from repository to bin resource folder
COPY_CMD = $$copyResources($${RESOURCE_FILES})
QMAKE_POST_LINK += $${COPY_CMD}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = $${HEADERS}
header_files.path = $${MNE_INSTALL_INCLUDE_DIR}/disp

INSTALLS += header_files

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

contains(MNECPP_CONFIG, withCodeCov) {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

# Deploy library in non-static builds only
win32:!contains(MNECPP_CONFIG, static) {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployLibArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}

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
