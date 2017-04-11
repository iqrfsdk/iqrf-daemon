project(UdpMessaging)

set(UdpMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessaging.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessagingTransaction.cpp
)

set(UdpMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/crc.h
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessage.h
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessaging.h
	${CMAKE_CURRENT_SOURCE_DIR}/UdpMessagingTransaction.h
)

add_library(${PROJECT_NAME} STATIC ${UdpMessaging_SRC_FILES} ${UdpMessaging_INC_FILES})
