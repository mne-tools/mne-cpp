cmake_minimum_required(VERSION 3.15)
project(mne_cpp_qt_probe LANGUAGES CXX)

if(NOT DEFINED MNE_QT_PROBE_CANDIDATE OR MNE_QT_PROBE_CANDIDATE STREQUAL "")
    message(FATAL_ERROR "MNE_QT_PROBE_CANDIDATE must be provided.")
endif()

if(NOT DEFINED MNE_QT_PROBE_VERSION OR MNE_QT_PROBE_VERSION STREQUAL "")
    message(FATAL_ERROR "MNE_QT_PROBE_VERSION must be provided.")
endif()

if(NOT DEFINED MNE_QT_PROBE_LINKAGE OR MNE_QT_PROBE_LINKAGE STREQUAL "")
    message(FATAL_ERROR "MNE_QT_PROBE_LINKAGE must be provided.")
endif()

if(NOT DEFINED MNE_QT_PROBE_RESULT_FILE OR MNE_QT_PROBE_RESULT_FILE STREQUAL "")
    message(FATAL_ERROR "MNE_QT_PROBE_RESULT_FILE must be provided.")
endif()

if(NOT DEFINED MNE_QT_PROBE_NO_OPENGL)
    set(MNE_QT_PROBE_NO_OPENGL ON)
endif()

set(_qt_candidate "${MNE_QT_PROBE_CANDIDATE}")

if(EXISTS "${_qt_candidate}/Qt6Config.cmake")
    set(Qt6_DIR "${_qt_candidate}")
elseif(EXISTS "${_qt_candidate}/lib/cmake/Qt6/Qt6Config.cmake")
    set(Qt6_DIR "${_qt_candidate}/lib/cmake/Qt6")
elseif(EXISTS "${_qt_candidate}/lib64/cmake/Qt6/Qt6Config.cmake")
    set(Qt6_DIR "${_qt_candidate}/lib64/cmake/Qt6")
elseif(EXISTS "${_qt_candidate}/share/qt6/cmake/Qt6/Qt6Config.cmake")
    set(Qt6_DIR "${_qt_candidate}/share/qt6/cmake/Qt6")
endif()

list(PREPEND CMAKE_PREFIX_PATH "${_qt_candidate}")

set(_mne_qt_components
    Core
    Concurrent
    Gui
    Network
    PrintSupport
    Svg
    Widgets
    Xml
    ShaderTools
)

find_package(Qt6 ${MNE_QT_PROBE_VERSION} EXACT REQUIRED COMPONENTS ${_mne_qt_components})

set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)

if(NOT MNE_QT_PROBE_NO_OPENGL)
    find_package(Qt6 REQUIRED COMPONENTS OpenGL OpenGLWidgets)
endif()

get_target_property(_qt_core_type Qt6::Core TYPE)

if(MNE_QT_PROBE_LINKAGE STREQUAL "static")
    if(NOT _qt_core_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR
            "Qt ${Qt6_VERSION} was found, but Qt6::Core is ${_qt_core_type}. "
            "A static Qt is required for --linkage=static.")
    endif()
elseif(MNE_QT_PROBE_LINKAGE STREQUAL "dynamic")
    if(_qt_core_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR
            "Qt ${Qt6_VERSION} was found, but Qt6::Core is static. "
            "A shared Qt is required for --linkage=dynamic.")
    endif()
else()
    message(FATAL_ERROR "Unsupported linkage mode: ${MNE_QT_PROBE_LINKAGE}")
endif()

get_filename_component(_qt_root_dir "${Qt6_DIR}/../../.." ABSOLUTE)

file(WRITE "${MNE_QT_PROBE_RESULT_FILE}"
    "QT_ROOT_DIR=${_qt_root_dir}\n"
    "QT_CMAKE_DIR=${Qt6_DIR}\n"
    "QT_CORE_TARGET_TYPE=${_qt_core_type}\n"
    "QT_VERSION=${Qt6_VERSION}\n"
)

