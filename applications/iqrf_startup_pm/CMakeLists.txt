project(iqrf_startup_pm)

enable_language(CXX)
enable_language(C)

cmake_minimum_required(VERSION 3.0)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

FIND_PACKAGE(iqrfd REQUIRED)
FIND_PACKAGE(clibcdc REQUIRED)
FIND_PACKAGE(clibspi REQUIRED)
FIND_PACKAGE(cutils REQUIRED)
FIND_PACKAGE(clibdpa REQUIRED)

#TODO paho finder is temporary there, remove when paho provides proper finder
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${cutils_CMAKE_SOURCE_DIR}/cmake/modules/")
FIND_PACKAGE(paho REQUIRED)

IF (PAHO_FOUND)
	MESSAGE(STATUS "Paho found: " ${PAHO_ROOT_DIR} )
ELSE()
	MESSAGE("Paho cannot be found")
ENDIF()

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>)

set(iqrf_startup_pm_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(iqrf_startup_pm_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/initModules.h
)

#file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
#file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _CONFIG_EX ${CMAKE_CURRENT_SOURCE_DIR}/configurationExamples/*)

#source_group("Header Files" FILES ${_HDRFILES})
#source_group("Source Files" FILES ${_SRCFILES})
source_group("Config" FILES ${_CONFIG})
source_group("ConfigExamples" FILES ${_CONFIG_EX})

# include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
# include_directories(${CMAKE_SOURCE_DIR}/Scheduler)
# include_directories(${CMAKE_SOURCE_DIR}/IqrfCdcChannel)
# include_directories(${CMAKE_SOURCE_DIR}/IqrfSpiChannel)
# include_directories(${CMAKE_SOURCE_DIR}/MessagingController)

# TODO
#include_directories(${iqrfd_INCLUDE_DIRS})
include_directories(${daemon_INCLUDE_DIRS})
include_directories(${cutils_INCLUDE_DIRS})

if (WIN32) 
	link_directories(${PAHO_LIBRARY_DIRS})
endif()

message("iqrf_startup_pm_SRC_FILES: " ${iqrf_startup_pm_SRC_FILES})

add_executable(${PROJECT_NAME} ${iqrf_startup_pm_INC_FILES} ${iqrf_startup_pm_SRC_FILES} ${_CONFIG} ${_CONFIG_EX})

if (WIN32) 
 	target_link_libraries(${PROJECT_NAME}
	Startup

	SimpleSerializer
	JsonSerializer
	MqMessaging
	MqttMessaging
	ClientServicePlain
	ClientServicePm

	MessagingController
	Scheduler
	paho-mqtt3a
	DpaExtension
	MqChannel
	IqrfCdcChannel
	IqrfSpiChannel
	Dpa
	UdpMessaging
	UdpChannel
	cdc
	spi_iqrf
	sysfs_gpio
	Ws2_32
	)
else()
	target_link_libraries(${PROJECT_NAME}
	Startup

	SimpleSerializer
	JsonSerializer
	MqMessaging
	MqttMessaging
	ClientServicePlain
	ClientServicePm

	MessagingController
	Scheduler
	paho-mqtt3as
	DpaExtension
	MqChannel
	IqrfCdcChannel
	IqrfSpiChannel
	Dpa
	UdpMessaging
	UdpChannel
	cdc
	spi_iqrf
	sysfs_gpio
	pthread
	rt
	)
endif()

if (WIN32)
set (_CONFIG_DEST ${CMAKE_CURRENT_BINARY_DIR})

else()
set (_CONFIG_DEST $<TARGET_FILE_DIR:${PROJECT_NAME}>)

INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

endif()

# do the copying
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configuration ${_CONFIG_DEST}/configuration
)
