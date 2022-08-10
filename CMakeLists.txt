cmake_minimum_required(VERSION 3.0.0)
project(USC VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(server server/main.cpp)
add_executable(client client/main.cpp)

find_package(LIBEVENT REQUIRED)

target_link_libraries(server libevent::core)
target_link_libraries(client libevent::core)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
