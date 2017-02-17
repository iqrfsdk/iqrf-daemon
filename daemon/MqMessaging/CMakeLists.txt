project(MqMessaging)

set(MqMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MqMessaging.cpp
)

set(MqMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MqMessaging.h
)

add_library(${PROJECT_NAME} STATIC ${MqMessaging_SRC_FILES} ${MqMessaging_INC_FILES})
