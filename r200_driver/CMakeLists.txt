cmake_minimum_required(VERSION 2.8.3)
project(r200_driver)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=gnu++0x)
endif()

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  sensor_msgs
)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# find_package(Boost REQUIRED COMPONENTS system)
# catkin_python_setup()

set(DSAPI_BINARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/libDSAPI.so")

################################################
## Declare ROS messages, services and actions ##
################################################


###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if you package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES r200_driver
  CATKIN_DEPENDS roscpp sensor_msgs OpenGL
  DEPENDS glut
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
# include_directories(include)
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS}
  ${GLUT_INCLUDE_DIRS}
)

## Declare a cpp executable
add_executable(interactive_capture src/samples/DSInteractiveCaptureGL.cpp)
add_executable(simple_capture src/samples/DSSimpleCaptureGL.cpp)

## Specify libraries to link a library or executable target against
target_link_libraries(interactive_capture ${catkin_LIBRARIES} ${DSAPI_BINARY_PATH} ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(simple_capture ${catkin_LIBRARIES} ${DSAPI_BINARY_PATH} ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})

#############
## Install ##
#############

#############
## Testing ##
#############

