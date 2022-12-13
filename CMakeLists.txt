cmake_minimum_required(VERSION 3.0.0)
project(USC VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(LibUServer STATIC server/UServer.cpp)
add_library(LibUClient STATIC client/UClient.cpp)

add_executable(server server/main.cpp)
add_executable(client client/main.cpp)

add_executable(BBE-client BigBrother'sEye/BBE-client.cpp)
add_executable(BBE-server BigBrother'sEye/BBE-server.cpp)

target_link_libraries(server LibUServer ws2_32)
target_link_libraries(client LibUClient ws2_32)

target_link_libraries(LibUServer ws2_32)
target_link_libraries(LibUClient ws2_32)

target_link_libraries(BBE-server LibUServer)
target_link_libraries(BBE-client LibUClient)

target_include_directories(BBE-server PUBLIC server)
target_include_directories(BBE-client PUBLIC client)
target_link_libraries(LibUClient wtsapi32)

set (CMAKE_CXX_STANDARD 20)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
