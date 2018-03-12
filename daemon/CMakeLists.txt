project (daemon)

cmake_minimum_required(VERSION 2.8.10)

set(DEFAULT_DAEMON_VERSION v1.1.0)
string(TIMESTAMP DEFAULT_BUILD_TIMESTAMP)

enable_language(CXX)
enable_language(C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

FIND_PACKAGE(clibcdc REQUIRED)
FIND_PACKAGE(clibspi REQUIRED)
FIND_PACKAGE(cutils REQUIRED)
FIND_PACKAGE(clibdpa REQUIRED)
FIND_PACKAGE(paho REQUIRED)

IF (PAHO_FOUND)
	MESSAGE(STATUS "Paho found: " ${PAHO_ROOT_DIR} )
ELSE()
	MESSAGE("Paho cannot be found")
ENDIF()

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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

message(STATUS "CMAKE_CXX_STANDARD:     ${CMAKE_CXX_STANDARD}")
message(STATUS "CMAKE_CXX_FLAGS:         ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG:   ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_C_FLAGS:           ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_C_FLAGS_RELEASE:   ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CMAKE_C_FLAGS_DEBUG:     ${CMAKE_C_FLAGS_DEBUG}")

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

include_directories(${clibcdc_INCLUDE_DIRS})
include_directories(${clibspi_INCLUDE_DIRS})
include_directories(${cutils_INCLUDE_DIRS})
include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${PAHO_INCLUDE_DIRS})

#library build
add_subdirectory(include)
add_subdirectory(Startup)
add_subdirectory(UdpMessaging)
add_subdirectory(Scheduler)
add_subdirectory(DaemonController)
add_subdirectory(iqrf_startup)

add_subdirectory(SimpleSerializer)
add_subdirectory(JsonSerializer)
add_subdirectory(MqMessaging)
add_subdirectory(MqttMessaging)
add_subdirectory(BaseService)

# Configure config file.
# This file specifies actions performed and variables exported when using find_package on this project.
# The find_package requires properly set ${PROJECT_NAME}_DIR variable to a location where the
# configured config file persists. This variable is usually specified from the outside of CMake.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/iqrfdConfig.cmake.in ${PROJECT_BINARY_DIR}/iqrfdConfig.cmake @ONLY)

if(DEFINED DAEMON_VERSION)
  message(STATUS "DAEMON_VERSION = " ${DAEMON_VERSION})
else()
  message(STATUS "Set to default DAEMON_VERSION = " ${DEFAULT_DAEMON_VERSION})
  set(DAEMON_VERSION ${DEFAULT_DAEMON_VERSION})
endif()

if(DEFINED BUILD_TIMESTAMP)
  message(STATUS "BUILD_TIMESTAMP = " ${BUILD_TIMESTAMP})
else()
  message(STATUS "Set to default BUILD_TIMESTAMP = " ${DEFAULT_BUILD_TIMESTAMP})
  set(BUILD_TIMESTAMP ${DEFAULT_BUILD_TIMESTAMP})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/VersionInfo.h.in ${CMAKE_BINARY_DIR}/VersionInfo.h @ONLY)
