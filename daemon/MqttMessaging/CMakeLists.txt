project(MqttMessaging)

set(MqttMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MqttMessaging.cpp
)

set(MqttMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MqttMessaging.h
)

add_library(${PROJECT_NAME} STATIC ${MqttMessaging_SRC_FILES} ${MqttMessaging_INC_FILES})
