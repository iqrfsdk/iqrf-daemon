project(Startup)

set(STARTUP_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Startup.cpp
)

set(STARTUP_INC_FILES
)

include_directories(${CMAKE_SOURCE_DIR}/DaemonController)

add_library(${PROJECT_NAME} STATIC ${STARTUP_SRC_FILES} ${STARTUP_INC_FILES})
