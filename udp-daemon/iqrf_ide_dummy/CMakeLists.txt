project(iqrf_ide_dummy)

set(iqrf_ide_dummy_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	#${CMAKE_SOURCE_DIR}/udp-daemon/MessageHandler.cpp
	${CMAKE_SOURCE_DIR}/udp-daemon/MessageQueue.cpp
	${CMAKE_SOURCE_DIR}/udp-daemon/UdpChannel.cpp
)

set(iqrf_ide_dummy_INC_FILES
	#${CMAKE_CURRENT_SOURCE_DIR}/MessageHandler.h
	${CMAKE_SOURCE_DIR}/udp-daemon/MessageQueue.h
	${CMAKE_SOURCE_DIR}/udp-daemon/Channel.h
	${CMAKE_SOURCE_DIR}/udp-daemon/UdpChannel.h
)

include_directories(${CMAKE_SOURCE_DIR}/udp)
include_directories(${CMAKE_SOURCE_DIR}/udp-daemon)

add_executable(${PROJECT_NAME} ${iqrf_ide_dummy_SRC_FILES} ${iqrf_ide_dummy_INC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} udp Ws2_32)
else()
	target_link_libraries(${PROJECT_NAME} udp pthread)
endif()

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
