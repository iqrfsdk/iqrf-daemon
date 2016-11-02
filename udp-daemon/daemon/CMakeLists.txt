project(daemon)

FIND_PACKAGE(clibcdc REQUIRED)
FIND_PACKAGE(clibspi REQUIRED)
FIND_PACKAGE(clibdpa REQUIRED)

set(daemon_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MessagingController.cpp
)

set(daemon_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MessagingController.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/UdpChannel)
include_directories(${CMAKE_SOURCE_DIR}/MqChannel)
include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
include_directories(${CMAKE_SOURCE_DIR}/PipeMessaging)
include_directories(${CMAKE_SOURCE_DIR}/DpaExtension)
include_directories(${CMAKE_SOURCE_DIR}/IqrfCdcChannel)
include_directories(${CMAKE_SOURCE_DIR}/IqrfSpiChannel)
include_directories(${clibcdc_INCLUDE_DIRS})
include_directories(${clibspi_INCLUDE_DIRS})
include_directories(${clibdpa_INCLUDE_DIRS})

MESSAGE("clibdpa_INCLUDE_DIRS: ${clibdpa_INCLUDE_DIRS}")

add_executable(${PROJECT_NAME} ${daemon_SRC_FILES} ${daemon_INC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} PipeMessaging MqChannel IqrfCdcChannel IqrfSpiChannel dpa UdpMessaging UdpChannel cdc spi_iqrf sysfs_gpio udp Ws2_32)
else()
	target_link_libraries(${PROJECT_NAME} PipeMessaging MqChannel IqrfCdcChannel IqrfSpiChannel dpa UdpMessaging UdpChannel cdc spi_iqrf sysfs_gpio udp pthread)
endif()

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
