project(UdpChannel)

set(UdpChannel_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/UdpChannel.cpp
)

set(UdpChannel_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/UdpChannel.h
)

add_library(${PROJECT_NAME} STATIC ${UdpChannel_SRC_FILES} ${UdpChannel_INC_FILES})

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
