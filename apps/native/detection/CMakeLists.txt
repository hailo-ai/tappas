cmake_minimum_required(VERSION 3.0.0)
project(detection_app)

add_executable(detection_app detection_app.cpp bitmap_utils.cpp yolov5_post_processing.cpp)
target_include_directories(detection_app PRIVATE "$ENV{TAPPAS_WORKSPACE}/hailort/sources/hailort/hailort/libhailort/include")

if(WIN32)
    set(HAILORT_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/hailort.lib")
else()
    set(HAILORT_LIB_PATH "libhailort.so")
endif()

find_package(Threads REQUIRED)
target_link_libraries(detection_app ${HAILORT_LIB_PATH} Threads::Threads)
