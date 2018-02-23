project(iqrfapp)

cmake_minimum_required(VERSION 3.0)

set(DEFAULT_IQRFAPP_VERSION v1.0.0)
string(TIMESTAMP DEFAULT_BUILD_TIMESTAMP)

if(DEFINED IQRFAPP_VERSION)
  message(STATUS "IQRFAPP_VERSION = " ${IQRFAPP_VERSION})
else()
  message(STATUS "Set to default IQRFAPP_VERSION = " ${DEFAULT_IQRFAPP_VERSION})
  set(IQRFAPP_VERSION ${DEFAULT_IQRFAPP_VERSION})
endif()

if(DEFINED BUILD_TIMESTAMP)
  message(STATUS "BUILD_TIMESTAMP = " ${BUILD_TIMESTAMP})
else()
  message(STATUS "Set to default BUILD_TIMESTAMP = " ${DEFAULT_BUILD_TIMESTAMP})
  set(BUILD_TIMESTAMP ${DEFAULT_BUILD_TIMESTAMP})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/VersionInfo.h.in ${CMAKE_BINARY_DIR}/VersionInfo.h @ONLY)

FIND_PACKAGE(iqrfd REQUIRED)
FIND_PACKAGE(cutils REQUIRED)

set(iqrfapp_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(iqrfapp_INC_FILES
	${CMAKE_BINARY_DIR}/VersionInfo.h
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
#include_directories(${iqrfd_INCLUDE_DIRS})
include_directories(${daemon_INCLUDE_DIRS})
include_directories(${cutils_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${iqrfapp_SRC_FILES} ${iqrfapp_INC_FILES})

if (WIN32)
	target_link_libraries(${PROJECT_NAME} MqChannel)
else()
	target_link_libraries(${PROJECT_NAME} MqChannel pthread rt)
endif()

if (WIN32)
set (_CONFIG_DEST ${CMAKE_CURRENT_BINARY_DIR})

else()
#set (_CONFIG_DEST $<TARGET_FILE_DIR:${PROJECT_NAME}>)
set (_CONFIG_DEST ${CMAKE_CURRENT_SOURCE_DIR}/../../daemon/build/Unix_Makefiles/bin )

INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

endif()

# do the copying
set (in_file ${CMAKE_CURRENT_SOURCE_DIR}/iqrfapp.json)
set (out_file ${_CONFIG_DEST}/configuration/iqrfapp.json)

if(${in_file} IS_NEWER_THAN ${out_file})
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${in_file} ${out_file}
)

endif()

# do the copying
set (in_file $<TARGET_FILE_DIR:${PROJECT_NAME}>/iqrfapp)
set (out_file ${_CONFIG_DEST}/iqrfapp)

if(${in_file} IS_NEWER_THAN ${out_file})
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${in_file} ${out_file}
)

endif()
