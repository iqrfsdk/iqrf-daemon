project(udp-daemon)

FIND_PACKAGE(clibcdc REQUIRED)
FIND_PACKAGE(clibspi REQUIRED)

set(udp-daemon_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MessageQueue.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MessageHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/IqrfCdcChannel.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/IqrfSpiChannel.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UdpChannel.cpp
)

set(udp-daemon_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MessageQueue.h
	${CMAKE_CURRENT_SOURCE_DIR}/MessageHandler.h
	${CMAKE_CURRENT_SOURCE_DIR}/Channel.h
	${CMAKE_CURRENT_SOURCE_DIR}/IqrfCdcChannel.h
	${CMAKE_CURRENT_SOURCE_DIR}/IqrfSpiChannel.h
	${CMAKE_CURRENT_SOURCE_DIR}/UdpChannel.h
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessage.h
)

include_directories(${CMAKE_SOURCE_DIR}/udp)
include_directories(${clibcdc_INCLUDE_DIRS})
include_directories(${clibspi_INCLUDE_DIRS})

message("clibspi_INCLUDE_DIRS: ${clibspi_INCLUDE_DIRS}")

add_executable(${PROJECT_NAME} ${udp-daemon_SRC_FILES} ${udp-daemon_INC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} cdc spi_iqrf sysfs_gpio udp Ws2_32)
else()
	target_link_libraries(${PROJECT_NAME} cdc spi_iqrf sysfs_gpio udp pthread)
endif()

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
