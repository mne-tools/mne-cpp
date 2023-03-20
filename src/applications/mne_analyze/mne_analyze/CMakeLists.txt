cmake_minimum_required(VERSION 3.14)
project(mne_analyze LANGUAGES CXX)

# Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Concurrent Network)

if(NOT WASM)
  list(APPEND QT_REQUIRED_COMPONENTS OpenGL 3DRender)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES main.cpp mainwindow.cpp analyzecore.cpp)

set(HEADERS info.h mainwindow.h analyzecore.h)

set(RESOURCES mne_analyze.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS}
                    ${RESOURCES})
else()
  add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED 
    mne_rtprocessing
    mne_connectivity
    mne_inverse
    mne_fwd
    mne_mne
    mne_fiff
    mne_fs
    mne_utils
    mne_events
    mne_disp
)

if(NOT WASM)
  list(APPEND MNE_LIBS_REQUIRED mne_disp3D)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
    anShared
)

if(NOT BUILD_SHARED_LIBS)
  set(MNE_AN_PLUGINS 
      analyze_averaging
      analyze_channelselection
      analyze_controlmanager
      analyze_coregistration
      analyze_dataloader
      analyze_datamanager
      analyze_dipolefit
      analyze_events
      analyze_filtering
      analyze_rawdataviewer
      analyze_sourcelocalization
  )
  
  if(NOT WASM)
    list(APPEND MNE_AN_PLUGINS analyze_view3d)
  endif()

  target_link_libraries(${PROJECT_NAME} PRIVATE ${MNE_AN_PLUGINS})

  target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER mne-cpp.org
             MACOSX_BUNDLE ${BUILD_MAC_APP_BUNDLE}
             WIN32_EXECUTABLE TRUE)

install(
  TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(${PROJECT_NAME})
endif()
