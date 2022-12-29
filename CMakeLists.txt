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

set(all server client BBE-server)

target_link_libraries(server LibUServer ws2_32)
target_link_libraries(client LibUClient ws2_32)

target_link_libraries(LibUServer ws2_32)
target_link_libraries(LibUClient ws2_32)

target_link_libraries(BBE-server LibUServer)
target_link_libraries(BBE-client LibUClient)

target_include_directories(BBE-server PUBLIC server)
target_include_directories(BBE-client PUBLIC client)
target_link_libraries(LibUClient wtsapi32)

target_link_libraries(BBE-server LibUServer)
target_link_libraries(BBE-client LibUClient pdh.dll)

target_include_directories(BBE-server PUBLIC server)
target_include_directories(BBE-client PUBLIC client)
target_link_libraries(LibUClient wtsapi32)

target_compile_features(server PRIVATE cxx_std_20)
target_compile_features(client PRIVATE cxx_std_20)
target_compile_features(BBE-server PRIVATE cxx_std_20)
target_compile_features(BBE-client PRIVATE cxx_std_20)
target_compile_features(LibUServer PRIVATE cxx_std_20)
target_compile_features(LibUClient PRIVATE cxx_std_20)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
