cmake_minimum_required(VERSION 3.14)
project(mne_disp3D LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Concurrent Widgets OpenGL 3DCore 3DRender 3DInput 3DExtras Charts)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

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

set(HEADERS
    engine/view/view3D.h
    engine/delegate/data3Dtreedelegate.h
    engine/model/data3Dtreemodel.h
    engine/model/items/subject/subjecttreeitem.h
    engine/model/items/measurement/measurementtreeitem.h
    engine/model/items/freesurfer/fssurfacetreeitem.h
    engine/model/items/freesurfer/fsannotationtreeitem.h
    engine/model/items/hemisphere/hemispheretreeitem.h
    engine/model/items/sourcespace/sourcespacetreeitem.h
    engine/model/items/sourcedata/mnedatatreeitem.h
    engine/model/items/sourcedata/ecddatatreeitem.h
    engine/model/items/network/networktreeitem.h
    engine/model/items/bem/bemtreeitem.h
    engine/model/items/bem/bemsurfacetreeitem.h
    engine/model/items/sensorspace/sensorsettreeitem.h
    engine/model/items/sensorspace/sensorsurfacetreeitem.h
    engine/model/items/sensorspace/sensorpositiontreeitem.h
    engine/model/items/digitizer/digitizertreeitem.h
    engine/model/items/digitizer/digitizersettreeitem.h
    engine/model/items/mri/mritreeitem.h
    engine/model/items/common/abstracttreeitem.h
    engine/model/items/common/abstract3Dtreeitem.h
    engine/model/items/common/metatreeitem.h
    engine/model/items/common/abstractmeshtreeitem.h
    engine/model/items/common/types.h
    engine/model/items/common/gpuinterpolationitem.h
    engine/model/workers/rtSourceLoc/rtsourcedataworker.h
    engine/model/workers/rtSourceLoc/rtsourcedatacontroller.h
    engine/model/workers/rtSourceLoc/rtsourceinterpolationmatworker.h
    engine/model/workers/rtSensorData/rtsensordataworker.h
    engine/model/workers/rtSensorData/rtsensordatacontroller.h
    engine/model/workers/rtSensorData/rtsensorinterpolationmatworker.h
    engine/model/3dhelpers/renderable3Dentity.h
    engine/model/3dhelpers/custommesh.h
    engine/model/items/common/types.h
    engine/model/materials/pervertexphongalphamaterial.h
    engine/model/materials/pervertextessphongalphamaterial.h
    engine/model/materials/shownormalsmaterial.h
    engine/model/materials/networkmaterial.h
    viewers/ecdview.h
    viewers/abstractview.h
    viewers/networkview.h
    viewers/sourceestimateview.h
    disp3D_global.h
    engine/model/items/sensordata/sensordatatreeitem.h
    helpers/interpolation/interpolation.h
    helpers/geometryinfo/geometryinfo.h
    engine/model/3dhelpers/geometrymultiplier.h
    engine/model/materials/geometrymultipliermaterial.h
    engine/view/customframegraph.h
    engine/model/materials/gpuinterpolationmaterial.h
    engine/model/materials/abstractphongalphamaterial.h
    engine/view/orbitalcameracontroller.h
)

set(FILE_TO_UPDATE disp3D_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")


add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set(FFTW_LIBS "")

if(USE_FFTW)
  if (WIN32)
    set(FFTW_LIBS
      ${FFTW_DIR_LIBS}/libfftw3-3.dll
      ${FFTW_DIR_LIBS}/libfftw3f-3.dll
      ${FFTW_DIR_LIBS}/libfftwf3l-3.dll
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE})
  elseif(UNIX AND NOT APPLE)
    set(FFTW_LIBS ${FFTW_DIR_LIBS}/lib/libfftw3.so)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE}/api)
  endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::3DCore
    Qt${QT_VERSION_MAJOR}::3DRender
    Qt${QT_VERSION_MAJOR}::3DInput
    Qt${QT_VERSION_MAJOR}::3DExtras
    Qt${QT_VERSION_MAJOR}::Charts
    eigen
    mne_utils
    mne_connectivity
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    mne_inverse
    mne_rtprocessing
    mne_disp
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_DISP3D_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
