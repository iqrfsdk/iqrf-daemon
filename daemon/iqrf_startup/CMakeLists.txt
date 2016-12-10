project(iqrf_startup)

FIND_PACKAGE(clibcdc REQUIRED)
FIND_PACKAGE(clibspi REQUIRED)
FIND_PACKAGE(clibdpa REQUIRED)

set(iqrf_startup_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(iqrf_startup_INC_FILES
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/UdpChannel)
include_directories(${CMAKE_SOURCE_DIR}/MqChannel)
include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
include_directories(${CMAKE_SOURCE_DIR}/MqMessaging)
include_directories(${CMAKE_SOURCE_DIR}/MqttMessaging)
include_directories(${CMAKE_SOURCE_DIR}/SchedulerMessaging)
include_directories(${CMAKE_SOURCE_DIR}/DpaExtension)
include_directories(${CMAKE_SOURCE_DIR}/IqrfCdcChannel)
include_directories(${CMAKE_SOURCE_DIR}/IqrfSpiChannel)
include_directories(${CMAKE_SOURCE_DIR}/SimpleSerializer)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer/include)
include_directories(${CMAKE_SOURCE_DIR}/TestClient)
include_directories(${CMAKE_SOURCE_DIR}/MessagingController)
include_directories(${clibcdc_INCLUDE_DIRS})
include_directories(${clibspi_INCLUDE_DIRS})
include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${PAHO_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${iqrf_startup_SRC_FILES} ${iqrf_startup_INC_FILES})

if (WIN32) 
 	target_link_libraries(${PROJECT_NAME}
	MessagingController
	TestClient
	JsonSerializer
	SimpleSerializer
	SchedulerMessaging
	MqttMessaging
	${PAHO_LIBRARIES}
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
	TestClient
	JsonSerializer
	SimpleSerializer
	SchedulerMessaging
	MqttMessaging
	${PAHO_LIBRARIES}
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

INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

