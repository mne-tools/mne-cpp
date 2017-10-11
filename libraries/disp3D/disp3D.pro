#--------------------------------------------------------------------------------------------------------------
#
# @file     disp3D.pro
# @author   Lorenz Esch <Lorenz.Esch@tu-ilmenauu.de>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     November, 2015
#
# @section  LICENSE
#
# Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT       += widgets 3dcore 3drender 3dinput 3dextras charts concurrent

DEFINES += DISP3DNEW_LIBRARY

TARGET = Disp3D
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
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Connectivityd \
            -lMNE$${MNE_LIB_VERSION}Dispd \

}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Connectivity \
            -lMNE$${MNE_LIB_VERSION}Disp \

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
    engine/view/view3D.cpp \
    engine/model/data3Dtreemodel.cpp \
    engine/model/data3Dtreedelegate.cpp \
    engine/model/items/subject/subjecttreeitem.cpp \
    engine/model/items/measurement/measurementtreeitem.cpp \
    engine/model/items/freesurfer/fssurfacetreeitem.cpp \
    engine/model/items/freesurfer/fsannotationtreeitem.cpp \
    engine/model/items/hemisphere/hemispheretreeitem.cpp \
    engine/model/items/sourcespace/sourcespacetreeitem.cpp \
    engine/model/items/sourceactivity/mneestimatetreeitem.cpp \
    engine/model/items/sourceactivity/ecddatatreeitem.cpp \
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
    engine/model/workers/rtSourceLoc/rtsourcelocdataworker.cpp \
    engine/model/workers/rtSensorData/rtsensordataworker.cpp \
    engine/model/3dhelpers/renderable3Dentity.cpp \
    engine/model/3dhelpers/custommesh.cpp \
    engine/model/materials/pervertexphongalphamaterial.cpp \
    engine/model/materials/pervertextessphongalphamaterial.cpp \
    engine/model/materials/shadermaterial.cpp \
    engine/model/materials/shownormalsmaterial.cpp \
    engine/model/materials/networkmaterial.cpp \
    engine/control/control3dwidget.cpp \
    adapters/ecdview.cpp \
    adapters/abstractview.cpp \
    adapters/networkview.cpp \
    engine/model/items/sensordata/sensordatatreeitem.cpp \
    helpers/interpolation/interpolation.cpp \
    helpers/geometryinfo/geometryinfo.cpp \
    engine/model/materials/instancedpositionrendermaterial.cpp \
    engine/model/3dhelpers/geometrymultiplier.cpp

HEADERS += \
    engine/view/view3D.h \
    engine/model/data3Dtreemodel.h \
    engine/model/data3Dtreedelegate.h \
    engine/model/items/subject/subjecttreeitem.h \
    engine/model/items/measurement/measurementtreeitem.h \
    engine/model/items/freesurfer/fssurfacetreeitem.h \
    engine/model/items/freesurfer/fsannotationtreeitem.h \
    engine/model/items/hemisphere/hemispheretreeitem.h \
    engine/model/items/sourcespace/sourcespacetreeitem.h \
    engine/model/items/sourceactivity/mneestimatetreeitem.h \
    engine/model/items/sourceactivity/ecddatatreeitem.h \
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
    engine/model/workers/rtSourceLoc/rtsourcelocdataworker.h \
    engine/model/workers/rtSensorData/rtsensordataworker.h \
    engine/model/3dhelpers/renderable3Dentity.h \
    engine/model/3dhelpers/custommesh.h \
    engine/model/items/common/types.h \
    engine/model/materials/pervertexphongalphamaterial.h \
    engine/model/materials/pervertextessphongalphamaterial.h \
    engine/model/materials/shadermaterial.h \
    engine/model/materials/shownormalsmaterial.h \
    engine/model/materials/networkmaterial.h \
    engine/control/control3dwidget.h \
    adapters/ecdview.h \
    adapters/abstractview.h \
    adapters/networkview.h \
    disp3D_global.h \
    engine/model/items/sensordata/sensordatatreeitem.h \
    helpers/interpolation/interpolation.h \
    helpers/geometryinfo/geometryinfo.h \
    engine/model/materials/instancedpositionrendermaterial.h \
    engine/model/3dhelpers/geometrymultiplier.h

FORMS += \
    engine/control/control3dwidget.ui \

RESOURCES += $$PWD/disp3d.qrc \

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = ./*.h
header_files.path = $${MNE_INCLUDE_DIR}/disp3D

INSTALLS += header_files

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
