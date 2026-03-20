cmake_minimum_required(VERSION 3.14)
project(mne_eximia2fiff LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    mne_utils
)

qt_finalize_target(${PROJECT_NAME})
