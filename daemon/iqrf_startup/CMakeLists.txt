project(iqrf_startup)

set(iqrf_startup_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/config.json
)

set(iqrf_startup_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/initModules.h
)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _CONFIG_WIN ${CMAKE_CURRENT_SOURCE_DIR}/configurationWin/*)
file(GLOB_RECURSE _CONFIG_EX ${CMAKE_CURRENT_SOURCE_DIR}/configurationExamples/*)
file(GLOB_RECURSE _SERVICE ${CMAKE_CURRENT_SOURCE_DIR}/service/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Config" FILES ${_CONFIG})
source_group("ConfigWin" FILES ${_CONFIG_WIN})
source_group("ConfigExamples" FILES ${_CONFIG_EX})
source_group("Service" FILES ${_SERVICE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
include_directories(${CMAKE_SOURCE_DIR}/Scheduler)
include_directories(${CMAKE_SOURCE_DIR}/IqrfCdcChannel)
include_directories(${CMAKE_SOURCE_DIR}/IqrfSpiChannel)

if (WIN32)
	link_directories(${PAHO_LIBRARY_DIRS})
endif()

add_executable(${PROJECT_NAME} ${_HDRFILES} ${_SRCFILES} ${_CONFIG} ${_CONFIG_WIN} ${_CONFIG_EX} ${_SERVICE})

if (WIN32)
 	target_link_libraries(${PROJECT_NAME}
	Startup

	SimpleSerializer
	JsonSerializer
	MqMessaging
	MqttMessaging
	BaseService

	DaemonController
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
	iphlpapi
	)
else()
	target_link_libraries(${PROJECT_NAME}
	Startup

	SimpleSerializer
	JsonSerializer
	MqMessaging
	MqttMessaging
	BaseService

	DaemonController
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

# configuration copy
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configuration ${_CONFIG_DEST}/configuration
)

# service copy
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/service ${_CONFIG_DEST}/service
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
