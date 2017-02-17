project(UdpMessaging)

set(UdpMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessaging.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/helpers.c
)

set(UdpMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/helpers.h
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessage.h
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessaging.h
)

include_directories(${CMAKE_SOURCE_DIR}/MessagingController)

add_library(${PROJECT_NAME} STATIC ${UdpMessaging_SRC_FILES} ${UdpMessaging_INC_FILES})

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
