project(TestClient)

FIND_PACKAGE(clibdpa REQUIRED)

set(TestClient_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/TestClient.cpp
)

set(TestClient_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/TestClient.h
)

include_directories(${CMAKE_SOURCE_DIR}/MqttMessaging)
include_directories(${CMAKE_SOURCE_DIR}/SimpleSerializer)
include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${TestClient_SRC_FILES} ${TestClient_INC_FILES})
