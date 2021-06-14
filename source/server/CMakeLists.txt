cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set(PROJECT_NAME LgFaceRecDemoTCP_Jetson_NanoV2)
project(${PROJECT_NAME})# LANGUAGES CXX CUDA)

set (CMAKE_CXX_STANDARD 11)

include(FindPkgConfig)

# OpenCV
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GIO2 REQUIRED gio-2.0)
pkg_check_modules(GOBJECT2 REQUIRED gobject-2.0)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(OPENCV REQUIRED opencv4)

find_package(CUDA)
message("-- CUDA version: ${CUDA_VERSION}")

set(
	CUDA_NVCC_FLAGS
	${CUDA_NVCC_FLAGS};
    -O3
	-gencode arch=compute_53,code=sm_53
	-gencode arch=compute_62,code=sm_62
)

if(CUDA_VERSION_MAJOR GREATER 9)
	message("-- CUDA ${CUDA_VERSION_MAJOR} detected, enabling SM_72")

	set(
		CUDA_NVCC_FLAGS
		${CUDA_NVCC_FLAGS};
		-gencode arch=compute_72,code=sm_72
	)

endif()

set(CMAKE_CXX_FLAGS  "-Wno-deprecated-declarations")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "")

# tensorRT
message("CUDA_TOOLKIT_ROOT_DIR = ${CUDA_TOOLKIT_ROOT_DIR}")

find_path(TENSORRT_INCLUDE_DIR NvInfer.h
  HINTS ${TENSORRT_ROOT} ${CUDA_TOOLKIT_ROOT_DIR} /usr/include
  PATH_SUFFIXES include)
find_path(TENSORRT_INCLUDE_DIR NvInferPlugin.h
  HINTS ${TENSORRT_ROOT} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES include)
MESSAGE(STATUS "Found TensorRT headers at ${TENSORRT_INCLUDE_DIR}")
find_library(TENSORRT_LIBRARY_INFER nvinfer
  HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 lib/x64 lib/aarch64-linux-gnu)
find_library(TENSORRT_LIBRARY_INFER_PLUGIN nvinfer_plugin
  HINTS  ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 lib/x64 lib/aarch64-linux-gnu)
  find_library(TENSORRT_LIBRARY_PARSER nvparsers
  HINTS  ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 lib/x64 lib/aarch64-linux-gnu)
set(TENSORRT_LIBRARY ${TENSORRT_LIBRARY_INFER} ${TENSORRT_LIBRARY_INFER_PLUGIN} ${TENSORRT_LIBRARY_PARSER})
message(STATUS "Find TensorRT libs at ${TENSORRT_LIBRARY}")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  TENSORRT DEFAULT_MSG TENSORRT_INCLUDE_DIR TENSORRT_LIBRARY)
if(NOT TENSORRT_FOUND)
  message(ERROR
    "Cannot find TensorRT library.")
endif()

# l2norm_helper plugin
add_subdirectory(trt_l2norm_helper)
include_directories(
  trt_l2norm_helper
  ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
  ${TENSORRT_INCLUDE_DIR}
  ${OBFUSCATOR_DIR}
)

include_directories(
  ${GLIB2_INCLUDE_DIRS}
  ${GIO2_INCLUDE_DIRS}
  ${GOBJECT2_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIRS}
  ${OPENCV_INCLUDE_DIRS}
  ../common
)

message("TENSORRT_LIBRARY = ${TENSORRT_LIBRARY}")

file(GLOB DIR_SRCS
        "src/*.cpp"
	"../common/Logger.cpp"
        "../common/NetworkTCP.cpp"
        "../common/TcpSendRecvJpeg.cpp"
        "../common/TcpSendRecvJpeg.cpp"
        "../common/openssl_hostname_validation.cpp"
)

file(COPY "../custom/keys/ca/ca.crt" "../custom/keys/server/server.crt" "../custom/keys/server/server.key"
    DESTINATION "keys"
)

message("DIR_SRCS = ${DIR_SRCS}")
cuda_add_executable(${PROJECT_NAME} ${DIR_SRCS})

add_library(libcertcheck STATIC IMPORTED)
set_target_properties(libcertcheck PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../common/libs/libcertcheck/libcertcheck.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../common/libs/libcertcheck/"
)

target_link_libraries(${PROJECT_NAME}
  libcertcheck
  trt_l2norm_helper
  ${TENSORRT_LIBRARY}
  ${GLIB2_LIBRARIES}
  ${GIO2_LIBRARIES}
  ${GOBJECT2_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${OPENCV_LIBRARIES}
)
