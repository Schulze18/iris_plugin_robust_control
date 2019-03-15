cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

find_package(roscpp REQUIRED)
find_package(std_msgs REQUIRED)
include_directories(${roscpp_INCLUDE_DIRS})
include_directories(${std_msgs_INCLUDE_DIRS})

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS}")


# Include header directory
# ----------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set sources
# ----------------------------------------------
set(
    robust_headers
    include/robust_gains.h
)


# Build our plugin
add_library(iris_plugin_robust_control SHARED src/iris_plugin_robust_control.cc ${robust_headers})
target_link_libraries(iris_plugin_robust_control ${GAZEBO_libraries} ${roscpp_LIBRARIES})
