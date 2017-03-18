project(MessagingController)

set(MC_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MessagingController.cpp
)

set(MC_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MessagingController.h
)

include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
include_directories(${CMAKE_SOURCE_DIR}/Scheduler)

add_library(${PROJECT_NAME} STATIC ${MC_SRC_FILES} ${MC_INC_FILES})
