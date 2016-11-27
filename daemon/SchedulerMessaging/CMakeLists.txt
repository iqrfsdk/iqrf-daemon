project(SchedulerMessaging)

FIND_PACKAGE(clibdpa REQUIRED)

set(SchedulerMessaging_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/SchedulerMessaging.cpp
)

set(SchedulerMessaging_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/SchedulerMessaging.h
)

include_directories(${CMAKE_SOURCE_DIR}/JsonCoding)
include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${SchedulerMessaging_SRC_FILES} ${SchedulerMessaging_INC_FILES})
