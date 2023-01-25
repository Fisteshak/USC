cmake_minimum_required(VERSION 3.0.0)
project(USC VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(thirdparty/fmt)
add_subdirectory(thirdparty/json-develop)
add_subdirectory(thirdparty/alpaca-master)

add_library(LibUServer STATIC server/UServer.cpp)
add_library(LibUClient STATIC client/UClient.cpp)
add_library(GInt STATIC crypto/GInt.cpp)
add_library(prime STATIC crypto/prime.cpp)
add_library(key_exchg STATIC crypto/key_exchg.cpp)
add_library(aes STATIC crypto/aes.cpp)	# Создание статической библиотеки

add_executable(server server/main.cpp)
add_executable(client client/main.cpp)

add_executable(BBE-client BigBrother'sEye/BBE-client.cpp)
add_executable(BBE-server BigBrother'sEye/BBE-server.cpp)

target_link_libraries(server LibUServer ws2_32)
target_link_libraries(client LibUClient ws2_32)

target_link_libraries(LibUServer ws2_32)
target_link_libraries(LibUClient ws2_32 wtsapi32)

# Гривовская
target_link_libraries(key_exchg prime GInt)
target_link_libraries(prime GInt key_exchg)

target_link_libraries(LibUServer GInt prime key_exchg aes)
target_link_libraries(LibUClient GInt prime key_exchg aes)

target_link_libraries(BBE-server LibUServer fmt::fmt nlohmann_json::nlohmann_json alpaca)
target_link_libraries(BBE-client LibUClient pdh.dll fmt::fmt alpaca)

target_include_directories(BBE-server PUBLIC server)
target_include_directories(BBE-client PUBLIC client)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
