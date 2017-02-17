project(ClientServicePlain)

set(ClientServicePlain_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ClientServicePlain.cpp
)

set(ClientServicePlain_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ClientServicePlain.h
)

include_directories(${CMAKE_SOURCE_DIR}/MqttMessaging)
include_directories(${CMAKE_SOURCE_DIR}/SimpleSerializer)

add_library(${PROJECT_NAME} STATIC ${ClientServicePlain_SRC_FILES} ${ClientServicePlain_INC_FILES})
