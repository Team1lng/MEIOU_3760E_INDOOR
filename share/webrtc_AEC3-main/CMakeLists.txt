cmake_minimum_required (VERSION 3.1)
project (AEC3 VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fPIC -pthread -std=c++14")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC")

set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_definitions(-DWEBRTC_POSIX)

option (USE_AEC "Use AEC provided math implementation" ON)

# configure a header file to pass some of the cmake settings to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
    )
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_BINARY_DIR}/demo ${PROJECT_SOURCE_DIR}/base/abseil)

if (USE_AEC)
  include_directories (${PROJECT_SOURCE_DIR}/audio_processing ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/base)
  add_subdirectory (audio_processing)
  add_subdirectory (api)
  add_subdirectory (base)
endif (USE_AEC)


