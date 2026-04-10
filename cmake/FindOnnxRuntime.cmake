#.rst:
# FindOnnxRuntime
# ----------------
#
# Find the Microsoft ONNX Runtime C/C++ API.
#
# Imported targets
# ^^^^^^^^^^^^^^^^
#
# ``OnnxRuntime::OnnxRuntime``
#   The ONNX Runtime library (shared or static), if found.
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# ``OnnxRuntime_FOUND``          – True if ONNX Runtime was found.
# ``OnnxRuntime_INCLUDE_DIR``    – Include directory containing ``onnxruntime_cxx_api.h``.
# ``OnnxRuntime_LIBRARY``        – Path to the ONNX Runtime library.
# ``OnnxRuntime_VERSION``        – Version string (if detectable from the header).
#
# Hints
# ^^^^^
#
# Set ``ONNXRUNTIME_ROOT_DIR`` or ``OnnxRuntime_DIR`` to point to the
# extracted pre-built ONNX Runtime package.  The module looks in
# ``<root>/include`` for headers and ``<root>/lib`` for the library.

# -- Header search -----------------------------------------------------------
find_path(OnnxRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATH_SUFFIXES include
    HINTS
        "${ONNXRUNTIME_ROOT_DIR}"
        "${OnnxRuntime_DIR}"
        "$ENV{ONNXRUNTIME_ROOT_DIR}"
)

# -- Library search -----------------------------------------------------------
# Look for both shared and static variants; prefer shared on desktop, static for WASM.
if(WASM OR EMSCRIPTEN OR NOT BUILD_SHARED_LIBS)
    find_library(OnnxRuntime_LIBRARY
        NAMES onnxruntime
        PATH_SUFFIXES lib lib64
        HINTS
            "${ONNXRUNTIME_ROOT_DIR}"
            "${OnnxRuntime_DIR}"
            "$ENV{ONNXRUNTIME_ROOT_DIR}"
    )
else()
    find_library(OnnxRuntime_LIBRARY
        NAMES onnxruntime
        PATH_SUFFIXES lib lib64
        HINTS
            "${ONNXRUNTIME_ROOT_DIR}"
            "${OnnxRuntime_DIR}"
            "$ENV{ONNXRUNTIME_ROOT_DIR}"
    )
endif()

# -- Version detection --------------------------------------------------------
if(OnnxRuntime_INCLUDE_DIR AND EXISTS "${OnnxRuntime_INCLUDE_DIR}/onnxruntime_c_api.h")
    file(STRINGS "${OnnxRuntime_INCLUDE_DIR}/onnxruntime_c_api.h" _ort_ver_major
         REGEX "^#define[ \t]+ORT_API_VERSION[ \t]+[0-9]+")
    if(_ort_ver_major)
        string(REGEX REPLACE "^#define[ \t]+ORT_API_VERSION[ \t]+([0-9]+).*" "\\1" OnnxRuntime_VERSION "${_ort_ver_major}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OnnxRuntime
    REQUIRED_VARS OnnxRuntime_LIBRARY OnnxRuntime_INCLUDE_DIR
    VERSION_VAR   OnnxRuntime_VERSION
)

# -- Imported target ----------------------------------------------------------
if(OnnxRuntime_FOUND AND NOT TARGET OnnxRuntime::OnnxRuntime)
    # Detect whether the found library is static or shared
    get_filename_component(_ort_ext "${OnnxRuntime_LIBRARY}" EXT)
    if(_ort_ext STREQUAL ".a" OR _ort_ext STREQUAL ".lib" AND (WASM OR EMSCRIPTEN OR NOT BUILD_SHARED_LIBS))
        set(_ort_lib_type STATIC)
    else()
        set(_ort_lib_type SHARED)
    endif()

    add_library(OnnxRuntime::OnnxRuntime ${_ort_lib_type} IMPORTED)
    set_target_properties(OnnxRuntime::OnnxRuntime PROPERTIES
        IMPORTED_LOCATION             "${OnnxRuntime_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OnnxRuntime_INCLUDE_DIR}"
    )
    if(APPLE AND _ort_lib_type STREQUAL "SHARED")
        # macOS .dylib needs IMPORTED_SONAME for RPATH resolution
        get_filename_component(_ort_soname "${OnnxRuntime_LIBRARY}" NAME)
        set_target_properties(OnnxRuntime::OnnxRuntime PROPERTIES
            IMPORTED_SONAME "${_ort_soname}"
        )
    endif()
    unset(_ort_lib_type)
endif()

mark_as_advanced(OnnxRuntime_INCLUDE_DIR OnnxRuntime_LIBRARY)
