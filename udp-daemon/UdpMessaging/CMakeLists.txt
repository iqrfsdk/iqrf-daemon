project(UdpMessaging)

FIND_PACKAGE(clibdpa REQUIRED)

set(UdpMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessaging.cpp
)

set(UdpMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessage.h
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessaging.h
)

include_directories(${CMAKE_SOURCE_DIR}/udp)
include_directories(${CMAKE_SOURCE_DIR}/UdpChannel)
include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${UdpMessaging_SRC_FILES} ${UdpMessaging_INC_FILES})

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
