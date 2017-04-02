project(iqrf_ide_dummy)

cmake_minimum_required(VERSION 3.0)

FIND_PACKAGE(iqrfd REQUIRED)
FIND_PACKAGE(cutils REQUIRED)

set(iqrf_ide_dummy_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

if(NOT CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake|MSBuild)")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	if(COMPILER_SUPPORTS_CXX11)
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11")
	elseif(COMPILER_SUPPORTS_CXX0X)
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++0x")
	else()
	  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
	endif()
endif()

# TODO
include_directories(${daemon_INCLUDE_DIRS})
include_directories(${daemon_INCLUDE_DIRS}/../UdpMessaging)
include_directories(${cutils_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${iqrf_ide_dummy_SRC_FILES} ${iqrf_ide_dummy_INC_FILES})

if (WIN32)
	target_link_libraries(${PROJECT_NAME} UdpMessaging UdpChannel Ws2_32)
else()
	target_link_libraries(${PROJECT_NAME} UdpMessaging UdpChannel pthread)
endif()
