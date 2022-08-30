cmake_minimum_required(VERSION 3.0.0)
project(USC VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(LibUServer STATIC server/UServer.cpp)
add_library(LibUClient STATIC client/UClient.cpp)

add_executable(server server/main.cpp)
add_executable(client client/main.cpp)

find_package(LIBEVENT REQUIRED)

target_link_libraries(server libevent::core LibUServer ws2_32)
target_link_libraries(client libevent::core LibUClient ws2_32)
target_link_libraries(LibUServer libevent::core ws2_32)
target_link_libraries(LibUClient libevent::core ws2_32)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
