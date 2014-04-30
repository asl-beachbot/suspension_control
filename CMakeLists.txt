# %Tag(FULLTEXT)%
cmake_minimum_required(VERSION 2.8.3)
project(suspension_control)

## Find catkin and any catkin packages
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(catkin REQUIRED COMPONENTS roscpp rospy std_msgs geometry_msgs genmsg tf)
include_directories(${Eigen_INCLUDE_DIRS})

## Declare ROS messages and services
#add_message_files(DIRECTORY msg FILES xy_vector.msg scan_vector.msg scan_point.msg xy_point.msg beach_map.msg)
#add_service_files(DIRECTORY srv FILES InitLocalization.srv)

## Generate added messages and services
generate_messages(DEPENDENCIES std_msgs geometry_msgs)

##add executables

add_executable(suspension_control src/controller.cpp)
target_link_libraries(suspension_control ${catkin_LIBRARIES} serial)
add_dependencies(suspension_control suspension_control_gencpp)

## Declare a catkin package
catkin_package()

# %EndTag(FULLTEXT)%