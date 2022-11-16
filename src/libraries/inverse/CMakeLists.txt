cmake_minimum_required(VERSION 3.14)
project(mne_inverse LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent)

set(SOURCES
    hpiFit/hpidataupdater.cpp
    hpiFit/hpimodelparameters.cpp
    hpiFit/signalmodel.cpp
    inverse_global.cpp
    minimumNorm/minimumnorm.cpp
    rapMusic/rapmusic.cpp
    rapMusic/pwlrapmusic.cpp
    rapMusic/dipole.cpp
    dipoleFit/dipole_fit.cpp
    dipoleFit/dipole_fit_data.cpp
    dipoleFit/dipole_fit_settings.cpp
    dipoleFit/dipole_forward.cpp
    dipoleFit/ecd.cpp
    dipoleFit/ecd_set.cpp
    dipoleFit/guess_data.cpp
    c/mne_inverse_operator.cpp
    c/mne_meas_data.cpp
    c/mne_meas_data_set.cpp
    hpiFit/hpifit.cpp
    hpiFit/hpifitdata.cpp
    hpiFit/sensorset.cpp
)

set(FILE_TO_UPDATE inverse_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    eigen
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_INVERSE_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")
