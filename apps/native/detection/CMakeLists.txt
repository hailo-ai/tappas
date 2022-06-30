cmake_minimum_required(VERSION 3.0.0)
project(detection_app)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Yolo Postprocess Library
add_library(yolo_postprocess 
            $ENV{TAPPAS_WORKSPACE}/core/hailo/gstreamer/libs/postprocesses/detection/yolo_postprocess.cpp 
            $ENV{TAPPAS_WORKSPACE}/core/hailo/gstreamer/libs/postprocesses/detection/yolo_output.cpp)
target_include_directories(yolo_postprocess PRIVATE "$ENV{TAPPAS_WORKSPACE}/core/hailo/general")
target_include_directories(yolo_postprocess PRIVATE "$ENV{TAPPAS_WORKSPACE}/core/hailo/gstreamer/libs/postprocesses/")
target_include_directories(yolo_postprocess PRIVATE "$ENV{TAPPAS_WORKSPACE}/core/open_source/rapidjson")
target_link_libraries(yolo_postprocess stdc++fs)

add_executable(detection_app detection_app.cpp bitmap_utils.cpp )
target_include_directories(detection_app PRIVATE "$ENV{TAPPAS_WORKSPACE}/hailort/sources/hailort/hailort/libhailort/include")
target_include_directories(detection_app PRIVATE "$ENV{TAPPAS_WORKSPACE}/core/hailo/general")
target_include_directories(detection_app PRIVATE "$ENV{TAPPAS_WORKSPACE}/core/hailo/gstreamer/libs/postprocesses/detection")

if(WIN32)
    set(HAILORT_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/hailort.lib")
else()
    set(HAILORT_LIB_PATH "libhailort.so")
endif()

find_package(Threads REQUIRED)
target_link_libraries(detection_app yolo_postprocess ${HAILORT_LIB_PATH} Threads::Threads)
