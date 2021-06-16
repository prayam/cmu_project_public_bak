cmake_minimum_required(VERSION 3.0.0)

set (CLIENT_VERSION_MAJOR 1)
set (CLIENT_VERSION_MINOR 0)
set (CLIENT_VERSION_PATCH 0)
set (CLIENT_VERSION_STRING
        ${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_PATCH})

project (client)

include(FindPkgConfig)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fstack-protector")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s -z noexecstack -z relro -z now")
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "")

add_library(libcertcheck STATIC IMPORTED)
set_target_properties(libcertcheck PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../common/libs/libcertcheck/libcertcheck.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../common/libs/libcertcheck/"
)

# need 'sudo apt install cmake gcc g++ libssl-dev libglib2.0-dev libopencv-dev'
pkg_check_modules(GTKMM3 REQUIRED gtkmm-3.0)
pkg_check_modules(OPENCV REQUIRED opencv4)
pkg_check_modules(OPENSSL REQUIRED openssl)

link_directories(
        "${CMAKE_SOURCE_DIR}/../common/libs/libcertcheck"
        ${GTKMM3_LIBRARY_DIRS}
        ${OPENCV_LIBRARY_DIRS}
        ${OPENSSL_LIBRARY_DIRS}
)

file(GLOB CLIENT_SRC
        "src/*.h"
        "src/*.cpp"
        "${CMAKE_SOURCE_DIR}/../common/Logger.cpp"
        "${CMAKE_SOURCE_DIR}/../common/NetworkTCP.cpp"
        "${CMAKE_SOURCE_DIR}/../common/TcpSendRecvJpeg.cpp"
        "${CMAKE_SOURCE_DIR}/../common/openssl_hostname_validation.cpp"
)

include_directories(
        "src/"
        "${CMAKE_SOURCE_DIR}/../common/"
        "${CMAKE_SOURCE_DIR}/../common/libs/libcertcheck"
        ${GTKMM3_INCLUDE_DIRS}
        ${OPENCV_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIRS}
)

add_compile_options(
        ${GTKMM3_CFLAGS_OTHER}
        ${OPENCV_CFLAGS_OTHER}
        ${OPENSSL_CFLAGS_OTHER}
)

add_executable(client ${CLIENT_SRC})
target_link_libraries(client
        certcheck_x64
        ${GTKMM3_LIBRARIES}
        ${OPENCV_LIBRARIES}
        ${OPENSSL_LIBRARIES}
)

file(COPY "${CMAKE_SOURCE_DIR}/../common/keys/ca/ca.crt" "${CMAKE_SOURCE_DIR}/../common/keys/client/client.crt" "${CMAKE_SOURCE_DIR}/../common/keys/client/client.key"
    DESTINATION "keys"
)