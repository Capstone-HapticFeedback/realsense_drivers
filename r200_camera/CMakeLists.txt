cmake_minimum_required(VERSION 2.8.3)
project(r200_camera)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=gnu++0x)
endif()

find_package(catkin REQUIRED COMPONENTS
  roscpp
  roslib
  sensor_msgs  
)

## System dependencies are found with CMake's conventions
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
link_directories(${GLFW_LIBRARY_DIRS})
find_package(OpenGL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

set(REALSENSE_BINARY_PATH "/usr/lib/librealsense.so")
set(REALSENSE_INCLUDE_DIRS "/usr/include/librealsense")



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
  LIBRARIES r200_camera
  CATKIN_DEPENDS roscpp sensor_msgs
  DEPENDS OpenGL glfw3
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
# include_directories(include)
include_directories(
  include
  ${OPENGL_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${REALSENSE_INCLUDE_DIRS}
  ${catkin_INCLUDE_DIRS}
)

## Declare a C++ library
# add_library(r200_camera
#   src/${PROJECT_NAME}/r200_camera.cpp
# )

## Add cmake target dependencies of the library
## as an example, code may need to be generated before libraries
## either from message generation or dynamic reconfigure
# add_dependencies(r200_camera ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Declare a C++ executable
add_executable(alignimages src/examples/alignimages.cpp)
add_executable(capture src/examples/capture.cpp)
add_executable(multicam src/examples/multicam.cpp)
add_executable(pointcloud src/examples/pointcloud.cpp)
add_executable(restart src/examples/restart.cpp)

## Add cmake target dependencies of the executable
## same as for the library above
# add_dependencies(r200_camera_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Specify libraries to link a library or executable target against
target_link_libraries(alignimages ${catkin_LIBRARIES} ${REALSENSE_BINARY_PATH} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} )
target_link_libraries(capture ${catkin_LIBRARIES} ${REALSENSE_BINARY_PATH} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} )
target_link_libraries(multicam ${catkin_LIBRARIES} ${REALSENSE_BINARY_PATH} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} )
target_link_libraries(pointcloud ${catkin_LIBRARIES} ${REALSENSE_BINARY_PATH} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} )
target_link_libraries(restart ${catkin_LIBRARIES} ${REALSENSE_BINARY_PATH} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} )

#############
## Install ##
#############


#############
## Testing ##
#############

