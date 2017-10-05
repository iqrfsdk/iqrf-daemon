project(iqrf_startup_example)

set(_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/initModules.h
)

file(GLOB_RECURSE _CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)

source_group("Config" FILES ${_CONFIG})

if (WIN32) 
	link_directories(${PAHO_LIBRARY_DIRS})
endif()

add_executable(${PROJECT_NAME} ${_INC_FILES} ${_SRC_FILES} ${_CONFIG})

if (WIN32) 
 	target_link_libraries(${PROJECT_NAME}
	Startup

	JsonSerializer
	MqttMessaging
	ServiceExample

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
	)
else()
	target_link_libraries(${PROJECT_NAME}
	Startup

	JsonSerializer
	MqttMessaging
	ServiceExample

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

# do the copying
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configuration ${_CONFIG_DEST}/configuration
)
