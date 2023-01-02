cmake_minimum_required(VERSION 3.0.0)
project(USC VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.3.1/mingw_64")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        ServerVis/main.cpp
        ServerVis/mainwindow.cpp
        ServerVis/mainwindow.h
        ServerVis/mainwindow.ui
)

        add_executable(ServerVis
            ${PROJECT_SOURCES}
        )

target_link_libraries(ServerVis PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(ServerVis PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ServerVis)
endif()


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

target_link_libraries(BBE-server LibUServer)
target_link_libraries(BBE-client LibUClient pdh.dll)

target_include_directories(BBE-server PUBLIC server)
target_include_directories(BBE-client PUBLIC client)
target_link_libraries(LibUClient wtsapi32)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
