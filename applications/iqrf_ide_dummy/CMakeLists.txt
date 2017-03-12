project(iqrf_ide_dummy)

cmake_minimum_required(VERSION 3.0)

FIND_PACKAGE(iqrfd REQUIRED)
FIND_PACKAGE(cutils REQUIRED)

set(iqrf_ide_dummy_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# TODO
#include_directories(${iqrfd_INCLUDE_DIRS})
include_directories(${daemon_INCLUDE_DIRS})
message("daemon_INCLUDE_DIRS: " ${daemon_INCLUDE_DIRS})
include_directories(${cutils_INCLUDE_DIRS})

# include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# include_directories(${CMAKE_SOURCE_DIR}/UdpChannel)
# include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)

add_executable(${PROJECT_NAME} ${iqrf_ide_dummy_SRC_FILES} ${iqrf_ide_dummy_INC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} UdpMessaging UdpChannel Ws2_32)
else()
	target_link_libraries(${PROJECT_NAME} UdpMessaging UdpChannel pthread)
endif()
