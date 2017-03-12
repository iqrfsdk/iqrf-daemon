project(UdpMessaging)

set(UdpMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessaging.cpp
)

set(UdpMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessaging.h
)

include_directories(${CMAKE_SOURCE_DIR}/MessagingController)

add_library(${PROJECT_NAME} STATIC ${UdpMessaging_SRC_FILES} ${UdpMessaging_INC_FILES})
