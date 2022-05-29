cmake_minimum_required (VERSION 3.8)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/modules/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

project (RouteBot)

find_package(RtAudio CONFIG REQUIRED)

set(ROUTEBOT_SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(ROUTEBOT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(ROUTEBOT_MODULE_DIR "${PROJECT_SOURCE_DIR}/modules")

set(BUILD_SHARED_LIBS OFF)

add_subdirectory("${ROUTEBOT_MODULE_DIR}/DPP")

file(GLOB ROUTEBOT_SRC_FILES "${ROUTEBOT_SRC_DIR}/*.cpp")
file(GLOB ROUTEBOT_HEADER_FILES "${ROUTEBOT_HEADER_DIR}/*.h")

add_executable(RouteBot "${ROUTEBOT_SRC_FILES}" "${ROUTEBOT_HEADER_FILES}")

set_property(TARGET RouteBot PROPERTY CXX_STANDARD 17)

target_include_directories(RouteBot PUBLIC "${ROUTEBOT_INCLUDE_DIR}")
target_include_directories(RouteBot PUBLIC $<TARGET_PROPERTY:dpp,INCLUDE_DIRECTORIES>)

target_link_libraries(RouteBot PRIVATE dpp)
target_link_libraries(RouteBot PRIVATE RtAudio::rtaudio)