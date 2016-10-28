project(MessageQueue)

set(MessageQueue_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MessageQueue.cpp
)

set(MessageQueue_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MessageQueue.h
)

add_library(${PROJECT_NAME} STATIC ${MessageQueue_SRC_FILES} ${MessageQueue_INC_FILES})

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
