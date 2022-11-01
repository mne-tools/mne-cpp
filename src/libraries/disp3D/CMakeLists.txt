cmake_minimum_required(VERSION 3.14)
project(mne_disp3D LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent Widgets OpenGl 3dCore 3dRender 3dInput 3dExtras Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Widgets OpenGl)

set(SOURCES
    disp3D_global.cpp
    engine/view/view3D.cpp
    engine/delegate/data3Dtreedelegate.cpp
    engine/model/data3Dtreemodel.cpp
    engine/model/items/subject/subjecttreeitem.cpp
    engine/model/items/measurement/measurementtreeitem.cpp
    engine/model/items/freesurfer/fssurfacetreeitem.cpp
    engine/model/items/freesurfer/fsannotationtreeitem.cpp
    engine/model/items/hemisphere/hemispheretreeitem.cpp
    engine/model/items/sourcespace/sourcespacetreeitem.cpp
    engine/model/items/sourcedata/mnedatatreeitem.cpp
    engine/model/items/sourcedata/ecddatatreeitem.cpp
    engine/model/items/network/networktreeitem.cpp
    engine/model/items/bem/bemtreeitem.cpp
    engine/model/items/bem/bemsurfacetreeitem.cpp
    engine/model/items/sensorspace/sensorsettreeitem.cpp
    engine/model/items/sensorspace/sensorsurfacetreeitem.cpp
    engine/model/items/sensorspace/sensorpositiontreeitem.cpp
    engine/model/items/digitizer/digitizertreeitem.cpp
    engine/model/items/digitizer/digitizersettreeitem.cpp
    engine/model/items/mri/mritreeitem.cpp
    engine/model/items/common/abstracttreeitem.cpp
    engine/model/items/common/abstract3Dtreeitem.cpp
    engine/model/items/common/metatreeitem.cpp
    engine/model/items/common/abstractmeshtreeitem.cpp
    engine/model/items/common/gpuinterpolationitem.cpp
    engine/model/workers/rtSourceLoc/rtsourcedataworker.cpp
    engine/model/workers/rtSourceLoc/rtsourcedatacontroller.cpp
    engine/model/workers/rtSourceLoc/rtsourceinterpolationmatworker.cpp
    engine/model/workers/rtSensorData/rtsensordataworker.cpp
    engine/model/workers/rtSensorData/rtsensordatacontroller.cpp
    engine/model/workers/rtSensorData/rtsensorinterpolationmatworker.cpp
    engine/model/3dhelpers/renderable3Dentity.cpp
    engine/model/3dhelpers/custommesh.cpp
    engine/model/materials/pervertexphongalphamaterial.cpp
    engine/model/materials/pervertextessphongalphamaterial.cpp
    engine/model/materials/shownormalsmaterial.cpp
    engine/model/materials/networkmaterial.cpp
    viewers/ecdview.cpp
    viewers/abstractview.cpp
    viewers/networkview.cpp
    viewers/sourceestimateview.cpp
    engine/model/items/sensordata/sensordatatreeitem.cpp
    helpers/interpolation/interpolation.cpp
    helpers/geometryinfo/geometryinfo.cpp
    engine/model/3dhelpers/geometrymultiplier.cpp
    engine/model/materials/geometrymultipliermaterial.cpp
    engine/view/customframegraph.cpp
    engine/model/materials/gpuinterpolationmaterial.cpp
    engine/model/materials/abstractphongalphamaterial.cpp
    engine/view/orbitalcameracontroller.cpp
)

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGl
    Qt${QT_VERSION_MAJOR}::3dCore
    Qt${QT_VERSION_MAJOR}::3dRender
    Qt${QT_VERSION_MAJOR}::3dInput
    Qt${QT_VERSION_MAJOR}::3dExtras
    Qt${QT_VERSION_MAJOR}::Charts
    eigen
    utils
    connectivity
    fiff
    fs
    mne
    fwd
    inverse
    rtprocessing
    disp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE mne_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")