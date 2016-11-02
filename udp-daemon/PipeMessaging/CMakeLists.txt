project(PipeMessaging)

FIND_PACKAGE(clibdpa REQUIRED)

set(PipeMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/PipeMessaging.cpp
)

set(PipeMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/PipeMessaging.h
)

include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${PipeMessaging_SRC_FILES} ${PipeMessaging_INC_FILES})
