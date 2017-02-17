project(Scheduler)

set(Scheduler_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Scheduler.cpp
)

set(Scheduler_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Scheduler.h
)

include_directories(${CMAKE_SOURCE_DIR}/SimpleSerializer)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer/include)

add_library(${PROJECT_NAME} STATIC ${Scheduler_SRC_FILES} ${Scheduler_INC_FILES})
