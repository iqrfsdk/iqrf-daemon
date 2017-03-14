project(iqrfapp)

cmake_minimum_required(VERSION 3.0)

FIND_PACKAGE(iqrfd REQUIRED)
FIND_PACKAGE(cutils REQUIRED)

set(iqrfapp_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(iqrfapp_INC_FILES
)

# TODO
#include_directories(${iqrfd_INCLUDE_DIRS})
include_directories(${daemon_INCLUDE_DIRS})
include_directories(${cutils_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${iqrfapp_SRC_FILES} ${iqrfapp_INC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} MqChannel )
else()
	target_link_libraries(${PROJECT_NAME} MqChannel pthread rt)
endif()
