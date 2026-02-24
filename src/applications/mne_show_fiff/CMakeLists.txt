cmake_minimum_required(VERSION 3.14)
project(mne_show_fiff LANGUAGES CXX)

# Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Network)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED
    COMPONENTS ${QT_REQUIRED_COMPONENTS}
)

set(SOURCES
    main.cpp
    mne_fiff_exp.cpp
    mne_fiff_exp_set.cpp
    mne_show_fiff_settings.cpp
)

set(HEADERS
    mne_fiff_exp.h
    mne_fiff_exp_set.h
    mne_show_fiff_settings.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED 
    mne_mne
    mne_fiff
    mne_fs
    mne_utils
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

# Install
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications
)

# Copy resource files
set(RESOURCE_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/general/explanations)
file(MAKE_DIRECTORY ${RESOURCE_DIR})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_RESOURCES_DIR}/general/explanations/fiff_explanations.txt
    ${RESOURCE_DIR}/fiff_explanations.txt
    COMMENT "Copying fiff_explanations.txt to build directory"
)

if(APPLE)
    set_source_files_properties(${PROJECT_RESOURCES_DIR}/general/explanations/fiff_explanations.txt
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/general/explanations")
    target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_RESOURCES_DIR}/general/explanations/fiff_explanations.txt)
endif()

install(FILES ${PROJECT_RESOURCES_DIR}/general/explanations/fiff_explanations.txt
    DESTINATION bin/resources/general/explanations
    COMPONENT applications
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
