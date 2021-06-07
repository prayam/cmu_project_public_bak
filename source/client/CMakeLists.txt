cmake_minimum_required(VERSION 2.8.7)

set (CLIENT_VERSION_MAJOR 1)
set (CLIENT_VERSION_MINOR 0)
set (CLIENT_VERSION_PATCH 0)
set (CLIENT_VERSION_STRING
        ${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_PATCH})

project (client)

include(FindPkgConfig)

# need 'sudo apt install cmake gcc g++ libssl-dev libglib2.0-dev libopencv-dev'
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GIO2 REQUIRED gio-2.0)
pkg_check_modules(GOBJECT2 REQUIRED gobject-2.0)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(OPENCV REQUIRED opencv4)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

file(GLOB CLIENT_SRC
        "src/*.h"
        "src/*.cpp"
        "../Common/NetworkTCP.cpp"
        "../Common/TcpSendRecvJpeg.cpp"
)

include_directories(
        "src/"
        "../Common/"
        ${GLIB2_INCLUDE_DIRS}
        ${GIO2_INCLUDE_DIRS}
        ${GOBJECT2_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIRS}
        ${OPENCV_INCLUDE_DIRS}
)
add_compile_options(
        ${GLIB2_CFLAGS_OTHER}
        ${GIO2_CFLAGS_OTHER}
        ${GOBJECT2_CFLAGS_OTHER}
        ${OPENSSL_CFLAGS_OTHER}
        ${OPENCV_CFLAGS_OTHER}
)

add_executable(client ${CLIENT_SRC})
target_link_libraries(client
        ${GLIB2_LIBRARIES}
        ${GIO2_LIBRARIES}
        ${GOBJECT2_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${OPENCV_LIBRARIES}
)