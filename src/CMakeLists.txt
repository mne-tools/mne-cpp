option(NO_EXAMPLES "Skip building examples" OFF)
option(NO_APPLICATIONS "Skip building applications" OFF)
option(NO_TESTS "Skip building tests" OFF)

option(LIB_ONLY "Build only the libraries. Equivalent to NO_EXAMPLES NO_APPLICATIONS and NO_TESTS" OFF)

add_subdirectory(libraries)
add_subdirectory(external)

if(NOT NO_APPLICATIONS)
    add_subdirectory(applications)
endif()

if(NOT NO_EXAMPLES)
    add_subdirectory(examples)
endif()

if(NOT NO_TESTS)
    add_subdirectory(testframes)
endif()
