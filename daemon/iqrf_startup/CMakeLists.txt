project(iqrf_startup)

FIND_PACKAGE(clibcdc REQUIRED)
FIND_PACKAGE(clibspi REQUIRED)
FIND_PACKAGE(clibdpa REQUIRED)

set(iqrf_startup_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/config.json
)

set(iqrf_startup_INC_FILES
)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _CONFIG_EX ${CMAKE_CURRENT_SOURCE_DIR}/configurationExamples/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Config" FILES ${_CONFIG})
source_group("ConfigExamples" FILES ${_CONFIG_EX})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/UdpChannel)
include_directories(${CMAKE_SOURCE_DIR}/MqChannel)
include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
include_directories(${CMAKE_SOURCE_DIR}/MqMessaging)
include_directories(${CMAKE_SOURCE_DIR}/MqttMessaging)
include_directories(${CMAKE_SOURCE_DIR}/Scheduler)
include_directories(${CMAKE_SOURCE_DIR}/DpaExtension)
include_directories(${CMAKE_SOURCE_DIR}/IqrfCdcChannel)
include_directories(${CMAKE_SOURCE_DIR}/IqrfSpiChannel)
include_directories(${CMAKE_SOURCE_DIR}/SimpleSerializer)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer/include)
include_directories(${CMAKE_SOURCE_DIR}/ClientServicePlain)
include_directories(${CMAKE_SOURCE_DIR}/ClientServicePm)
include_directories(${CMAKE_SOURCE_DIR}/MessagingController)
include_directories(${clibcdc_INCLUDE_DIRS})
include_directories(${clibspi_INCLUDE_DIRS})
include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${PAHO_INCLUDE_DIRS})

if (WIN32) 
	link_directories(${PAHO_LIBRARY_DIRS})
endif()

add_executable(${PROJECT_NAME} ${_HDRFILES} ${_SRCFILES} ${_CONFIG} ${_CONFIG_EX})

if (WIN32) 
 	target_link_libraries(${PROJECT_NAME}
	MessagingController
	ClientServicePm
	ClientServicePlain
	JsonSerializer
	SimpleSerializer
	Scheduler
	MqttMessaging
	paho-mqtt3as
	MqMessaging
	DpaExtension
	MqChannel
	IqrfCdcChannel
	IqrfSpiChannel
	dpa
	UdpMessaging
	UdpChannel
	cdc
	spi_iqrf
	sysfs_gpio
	Ws2_32
	)
else()
	target_link_libraries(${PROJECT_NAME}
	MessagingController
	ClientServicePm
	ClientServicePlain
	JsonSerializer
	SimpleSerializer
	Scheduler
	MqttMessaging
	paho-mqtt3as
	MqMessaging
	DpaExtension
	MqChannel
	IqrfCdcChannel
	IqrfSpiChannel
	dpa
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

# if (WIN32)
  # foreach( file_i ${_CONFIG})
    # add_custom_command(
    # TARGET ${PROJECT_NAME}
    # POST_BUILD
    # COMMAND ${CMAKE_COMMAND}
    # ARGS -E copy ${file_i} ${_CONFIG_DEST}/configuration/
    # )
  # endforeach( file_i )
# else()
    # add_custom_command(
    # TARGET ${PROJECT_NAME}
    # POST_BUILD
    # COMMAND ${CMAKE_COMMAND}
    # ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configuration ${_CONFIG_DEST}/configuration
    # )
# endif()

# INSTALL(TARGETS ${PROJECT_NAME}
    # RUNTIME DESTINATION bin
    # LIBRARY DESTINATION lib)
# endif()

