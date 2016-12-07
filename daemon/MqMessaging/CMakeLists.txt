project(MqMessaging)

FIND_PACKAGE(clibdpa REQUIRED)

set(MqMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MqMessaging.cpp
)

set(MqMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MqMessaging.h
)

include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${MqMessaging_SRC_FILES} ${MqMessaging_INC_FILES})
