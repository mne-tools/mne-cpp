#==============================================================================================================
#
# @file     disp3D.pro
# @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
#           Juan Garcia-Prieto <juangpc@gmail.com>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @since    0.1.0
# @date     November, 2015
#
# @section  LICENSE
#
# Copyright (C) 2015, Lars Debor, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
# @brief    This project file builds the new display 3D library which depends on the qt3d module.
#
#==============================================================================================================

include(../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT += widgets 3dcore 3drender 3dinput 3dextras charts concurrent opengl

DEFINES += DISP3D_LIBRARY

DESTDIR = $${MNE_LIBRARY_DIR}

TARGET = Disp3D
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
    LIBS += -lmnecppDispd \
            -lmnecppEventsd \
            -lmnecppRtProcessingd \
            -lmnecppConnectivityd \
            -lmnecppInversed \
            -lmnecppFwdd \
            -lmnecppMned \
            -lmnecppFiffd \
            -lmnecppFsd \
            -lmnecppUtilsd \
} else {
    LIBS += -lmnecppDisp \
            -lmnecppEvents \
            -lmnecppRtProcessing \
            -lmnecppConnectivity \
            -lmnecppInverse \
            -lmnecppFwd \
            -lmnecppMne \
            -lmnecppFiff \
            -lmnecppFs \
            -lmnecppUtils \
}

SOURCES += \
    disp3D_global.cpp \
    engine/view/view3D.cpp \
    engine/delegate/data3Dtreedelegate.cpp \
    engine/model/data3Dtreemodel.cpp \
    engine/model/items/subject/subjecttreeitem.cpp \
    engine/model/items/measurement/measurementtreeitem.cpp \
    engine/model/items/freesurfer/fssurfacetreeitem.cpp \
    engine/model/items/freesurfer/fsannotationtreeitem.cpp \
    engine/model/items/hemisphere/hemispheretreeitem.cpp \
    engine/model/items/sourcespace/sourcespacetreeitem.cpp \
    engine/model/items/sourcedata/mnedatatreeitem.cpp \
    engine/model/items/sourcedata/ecddatatreeitem.cpp \
    engine/model/items/network/networktreeitem.cpp \
    engine/model/items/bem/bemtreeitem.cpp \
    engine/model/items/bem/bemsurfacetreeitem.cpp \
    engine/model/items/sensorspace/sensorsettreeitem.cpp \
    engine/model/items/sensorspace/sensorsurfacetreeitem.cpp \
    engine/model/items/sensorspace/sensorpositiontreeitem.cpp \
    engine/model/items/digitizer/digitizertreeitem.cpp \
    engine/model/items/digitizer/digitizersettreeitem.cpp \
    engine/model/items/mri/mritreeitem.cpp \
    engine/model/items/common/abstracttreeitem.cpp \
    engine/model/items/common/abstract3Dtreeitem.cpp \
    engine/model/items/common/metatreeitem.cpp \
    engine/model/items/common/abstractmeshtreeitem.cpp \
    engine/model/items/common/gpuinterpolationitem.cpp \
    engine/model/workers/rtSourceLoc/rtsourcedataworker.cpp \
    engine/model/workers/rtSourceLoc/rtsourcedatacontroller.cpp \
    engine/model/workers/rtSourceLoc/rtsourceinterpolationmatworker.cpp \
    engine/model/workers/rtSensorData/rtsensordataworker.cpp \
    engine/model/workers/rtSensorData/rtsensordatacontroller.cpp \
    engine/model/workers/rtSensorData/rtsensorinterpolationmatworker.cpp \
    engine/model/3dhelpers/renderable3Dentity.cpp \
    engine/model/3dhelpers/custommesh.cpp \
    engine/model/materials/pervertexphongalphamaterial.cpp \
    engine/model/materials/pervertextessphongalphamaterial.cpp \
    engine/model/materials/shownormalsmaterial.cpp \
    engine/model/materials/networkmaterial.cpp \
    viewers/ecdview.cpp \
    viewers/abstractview.cpp \
    viewers/networkview.cpp \
    viewers/sourceestimateview.cpp \
    engine/model/items/sensordata/sensordatatreeitem.cpp \
    helpers/interpolation/interpolation.cpp \
    helpers/geometryinfo/geometryinfo.cpp \
    engine/model/3dhelpers/geometrymultiplier.cpp \
    engine/model/materials/geometrymultipliermaterial.cpp \
    engine/view/customframegraph.cpp \
    engine/model/materials/gpuinterpolationmaterial.cpp \
    engine/model/materials/abstractphongalphamaterial.cpp \
    engine/view/orbitalcameracontroller.cpp

HEADERS += \
    engine/view/view3D.h \
    engine/delegate/data3Dtreedelegate.h \
    engine/model/data3Dtreemodel.h \
    engine/model/items/subject/subjecttreeitem.h \
    engine/model/items/measurement/measurementtreeitem.h \
    engine/model/items/freesurfer/fssurfacetreeitem.h \
    engine/model/items/freesurfer/fsannotationtreeitem.h \
    engine/model/items/hemisphere/hemispheretreeitem.h \
    engine/model/items/sourcespace/sourcespacetreeitem.h \
    engine/model/items/sourcedata/mnedatatreeitem.h \
    engine/model/items/sourcedata/ecddatatreeitem.h \
    engine/model/items/network/networktreeitem.h \
    engine/model/items/bem/bemtreeitem.h \
    engine/model/items/bem/bemsurfacetreeitem.h \
    engine/model/items/sensorspace/sensorsettreeitem.h \
    engine/model/items/sensorspace/sensorsurfacetreeitem.h \
    engine/model/items/sensorspace/sensorpositiontreeitem.h \
    engine/model/items/digitizer/digitizertreeitem.h \
    engine/model/items/digitizer/digitizersettreeitem.h \
    engine/model/items/mri/mritreeitem.h \
    engine/model/items/common/abstracttreeitem.h \
    engine/model/items/common/abstract3Dtreeitem.h \
    engine/model/items/common/metatreeitem.h \
    engine/model/items/common/abstractmeshtreeitem.h \
    engine/model/items/common/types.h \
    engine/model/items/common/gpuinterpolationitem.h \
    engine/model/workers/rtSourceLoc/rtsourcedataworker.h \
    engine/model/workers/rtSourceLoc/rtsourcedatacontroller.h \
    engine/model/workers/rtSourceLoc/rtsourceinterpolationmatworker.h \
    engine/model/workers/rtSensorData/rtsensordataworker.h \
    engine/model/workers/rtSensorData/rtsensordatacontroller.h \
    engine/model/workers/rtSensorData/rtsensorinterpolationmatworker.h \
    engine/model/3dhelpers/renderable3Dentity.h \
    engine/model/3dhelpers/custommesh.h \
    engine/model/items/common/types.h \
    engine/model/materials/pervertexphongalphamaterial.h \
    engine/model/materials/pervertextessphongalphamaterial.h \
    engine/model/materials/shownormalsmaterial.h \
    engine/model/materials/networkmaterial.h \
    viewers/ecdview.h \
    viewers/abstractview.h \
    viewers/networkview.h \
    viewers/sourceestimateview.h \
    disp3D_global.h \
    engine/model/items/sensordata/sensordatatreeitem.h \
    helpers/interpolation/interpolation.h \
    helpers/geometryinfo/geometryinfo.h \
    engine/model/3dhelpers/geometrymultiplier.h \
    engine/model/materials/geometrymultipliermaterial.h \
    engine/view/customframegraph.h \
    engine/model/materials/gpuinterpolationmaterial.h \
    engine/model/materials/abstractphongalphamaterial.h \
    engine/view/orbitalcameracontroller.h

FORMS += \

RESOURCES += $$PWD/disp3d.qrc \

clang {
    QMAKE_CXXFLAGS += -isystem $${EIGEN_INCLUDE_DIR} 
} else {
    INCLUDEPATH += $${EIGEN_INCLUDE_DIR} 
}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

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

################################################## BUILD TIMESTAMP/HASH UPDATER ############################################

FILE_TO_UPDATE = disp3D_global.cpp
win32 {
    CONFIG(debug, debug|release) {
        OBJ_TARJET = debug\disp3D_global.obj
    } else {
        OBJ_TARJET = release\disp3D_global.obj
    }
}

ALL_FILES += $$HEADERS
ALL_FILES += $$SOURCES
ALL_FILES -= $$FILE_TO_UPDATE

FileUpdater.target = phonyFileUpdater
for (I_FILE, ALL_FILES) {
    FileUpdater.depends += $${PWD}/$${I_FILE}
}

unix|macx {
    FileUpdater.commands = touch $${PWD}/$${FILE_TO_UPDATE} ; echo PASTA > phonyFileUpdater
}

win32 {
    FileUpdater.commands = copy /y $$shell_path($${PWD})\\$${FILE_TO_UPDATE} +,, $$shell_path($${PWD})\\$${FILE_TO_UPDATE} & echo PASTA > phonyFileUpdater
    OrderForcerTarget.target = $${OBJ_TARJET}
    OrderForcerTarget.depends += phonyFileUpdater
    QMAKE_EXTRA_TARGETS += OrderForcerTarget
}

PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdater
