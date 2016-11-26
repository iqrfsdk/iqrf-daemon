project(MqttMessaging)

FIND_PACKAGE(clibdpa REQUIRED)
#FIND_PACKAGE(paho REQUIRED)

set(MqttMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MqttMessaging.cpp
)

set(MqttMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MqttMessaging.h
)

include_directories(${PAHO_INCLUDE_DIRS})
include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${MqttMessaging_SRC_FILES} ${MqttMessaging_INC_FILES})
