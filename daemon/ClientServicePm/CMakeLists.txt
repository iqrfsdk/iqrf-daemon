project(ClientServicePm)

FIND_PACKAGE(clibdpa REQUIRED)

set(ClientService_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ClientService.cpp
)

set(ClientService_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ClientService.h
)

include_directories(${CMAKE_SOURCE_DIR}/MqttMessaging)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer)
include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${ClientService_SRC_FILES} ${ClientService_INC_FILES})
