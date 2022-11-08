option(NO_EXAMPLES "Skip building examples" OFF)
option(NO_APPLICATIONS "Skip building applications" OFF)

add_subdirectory(libraries)
add_subdirectory(external)

if(NOT NO_APPLICATIONS)
    add_subdirectory(applications)
endif()

if(NOT NO_EXAMPLES)
    add_subdirectory(examples)
endif()
