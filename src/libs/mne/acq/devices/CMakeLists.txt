# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

# mne_acq_devices: vendor amplifiers as acquisition schemes, each with a simulator. Standard C++ only.
cmake_minimum_required(VERSION 3.21)
project(mne_acq_devices
  LANGUAGES CXX
)

set(SOURCES
  acq_devices.cpp
  brainamp/brainamp_backend.cpp
  brainamp/brainamp_simulator.cpp
  tmsi/tmsi_backend.cpp
  tmsi/tmsi_simulator.cpp
  gusbamp/gusbamp_backend.cpp
  gusbamp/gusbamp_simulator.cpp
  eego/eego_backend.cpp
  eego/eego_simulator.cpp
  eego/eego_driver.cpp
  neuromag/neuromag_backend.cpp
  neuromag/neuromag_fiff.cpp
  neuromag/neuromag_simulator.cpp
  polhemus/fastrak_record.cpp
  polhemus/polhemus_backend.cpp
  polhemus/polhemus_simulator.cpp
  ftbuffer/ftbuffer_backend.cpp
  ftbuffer/ftbuffer_client.cpp
  ftbuffer/ftbuffer_simulator.cpp
  babymeg/babymeg_backend.cpp
  babymeg/babymeg_client.cpp
  babymeg/babymeg_info.cpp
  babymeg/babymeg_simulator.cpp
  natus/natus_backend.cpp
  natus/natus_simulator.cpp
  rtserver/rtserver_backend.cpp
  rtserver/rtserver_client.cpp
  rtserver/rtserver_simulator.cpp
  brainflow/brainflow_backend.cpp
  brainflow/brainflow_library.cpp
  brainflow/brainflow_stream.cpp
  brainflow/streaming_board_backend.cpp
  brainflow/brainflow_file_backend.cpp
)

set(PRIVATE_HEADERS
  trigger_word.h
  brainamp/brainamp_transport.h
  tmsi/tmsi_transport.h
  gusbamp/gusbamp_transport.h
  eego/eego_transport.h
  neuromag/neuromag_fiff.h
  neuromag/neuromag_transport.h
  polhemus/fastrak_record.h
  polhemus/polhemus_transport.h
  ftbuffer/ftbuffer_transport.h
  babymeg/babymeg_transport.h
  natus/natus_transport.h
  rtserver/rtserver_transport.h
  brainflow/brainflow_library.h
  brainflow/brainflow_stream.h
  brainflow/brainflow_boards.inc
  transport_error.h
)

set(HEADERS
  acq_devices_global.h
  acq_devices.h
)

# Brain Products' BrainAmpIoCtl.h is not redistributable
set(MNE_BRAINAMP_SDK_DIR "${CMAKE_SOURCE_DIR}/src/applications/mne_scan/plugins/brainamp" CACHE PATH
    "Directory with BrainAmpIoCtl.h; the BrainAmp driver interface is built on Windows when it is there")
set(_brainamp_hardware OFF)
if(WIN32 AND EXISTS "${MNE_BRAINAMP_SDK_DIR}/BrainAmpIoCtl.h")
  set(_brainamp_hardware ON)
  list(APPEND SOURCES brainamp/brainamp_driver_win32.cpp)
endif()

# The TMSi SDK and the g.USBamp API are system DLLs, loaded at run time
if(WIN32)
  list(APPEND SOURCES tmsi/tmsi_driver_win32.cpp gusbamp/gusbamp_driver_win32.cpp)
endif()

# The Neuromag acquisition runs on Linux; its client uses POSIX sockets and shared memory
if(UNIX AND NOT EMSCRIPTEN)
  list(APPEND SOURCES neuromag/neuromag_dacq_posix.cpp)
endif()

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${PRIVATE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../..> $<INSTALL_INTERFACE:include>)
target_link_libraries(${PROJECT_NAME} PUBLIC mne_acq)
target_link_libraries(${PROJECT_NAME} PRIVATE mne_com)

if(_brainamp_hardware)
  target_include_directories(${PROJECT_NAME} PRIVATE "${MNE_BRAINAMP_SDK_DIR}")
  target_compile_definitions(${PROJECT_NAME} PUBLIC MNE_ACQ_BRAINAMP_HARDWARE)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ACQ_DEVICES_LIBRARY)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

foreach(_header ${HEADERS})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/acq/devices
        COMPONENT sdk
    )
endforeach()
