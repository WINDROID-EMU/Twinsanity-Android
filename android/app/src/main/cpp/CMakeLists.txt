cmake_minimum_required(VERSION 3.22.1)
project(TwinsanityAndroid LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(TWIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../" ABSOLUTE)
set(SDL3_DIR "${TWIN_ROOT}/build/android/sdl3-arm64-v8a")
set(FFMPEG_DIR "${TWIN_ROOT}/build/android/ffmpeg/android-arm64")

add_definitions(-D__ANDROID__ -DTWIN_NATIVE -DVK_USE_PLATFORM_ANDROID_KHR -DTWIN_PORT_VERSION="1.0.19")

find_library(VULKAN_LIB vulkan REQUIRED)
find_library(LOG_LIB log REQUIRED)
find_library(ANDROID_LIB android REQUIRED)

add_library(SDL3 SHARED IMPORTED)
set_target_properties(SDL3 PROPERTIES IMPORTED_LOCATION "${SDL3_DIR}/libSDL3.so")

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libavcodec.so")

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libavutil.so")

include_directories(
    "${TWIN_ROOT}/native/include"
    "${TWIN_ROOT}/include"
    "${TWIN_ROOT}/src"
    "${TWIN_ROOT}/src/platform/native"
    "${TWIN_ROOT}/src/platform/native/graphics"
    "${TWIN_ROOT}/android/third_party/sdl3/include"
    "${FFMPEG_DIR}/include"
)

file(GLOB GAME_SOURCES "${TWIN_ROOT}/src/game/*.cpp")
set(CORE_SOURCES "${TWIN_ROOT}/src/debug.cpp" "${TWIN_ROOT}/src/gcc2.cpp" "${TWIN_ROOT}/src/main.cpp")

file(STRINGS "${TWIN_ROOT}/native/reused_ps2.txt" REUSED_PS2 REGEX "^src/")
set(RETAIL_SOURCES "")
foreach(item ${REUSED_PS2})
    list(APPEND RETAIL_SOURCES "${TWIN_ROOT}/${item}")
endforeach()

file(GLOB_RECURSE PLATFORM_SOURCES "${TWIN_ROOT}/src/platform/native/*.cpp")
list(FILTER PLATFORM_SOURCES EXCLUDE REGEX ".*_win32\\.cpp$")
list(FILTER PLATFORM_SOURCES EXCLUDE REGEX ".*windows.*")

file(GLOB GENERATED_SOURCES "${TWIN_ROOT}/build/native/*.cpp")
list(REMOVE_ITEM GENERATED_SOURCES "${TWIN_ROOT}/build/native/probe.cpp")
list(APPEND GENERATED_SOURCES "${TWIN_ROOT}/build/android/stubs.cpp")

add_library(main SHARED
    ${GAME_SOURCES}
    ${CORE_SOURCES}
    ${RETAIL_SOURCES}
    ${PLATFORM_SOURCES}
    ${GENERATED_SOURCES}
)

target_link_libraries(main PRIVATE
    SDL3
    avcodec
    avutil
    ${VULKAN_LIB}
    ${LOG_LIB}
    ${ANDROID_LIB}
)
