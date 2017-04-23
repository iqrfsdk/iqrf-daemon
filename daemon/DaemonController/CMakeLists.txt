project(DaemonController)

set(MC_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/DaemonController.cpp
)

set(MC_INC_FILES
	${CMAKE_BINARY_DIR}/VersionInfo.h
	${CMAKE_CURRENT_SOURCE_DIR}/DaemonController.h
)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
include_directories(${CMAKE_SOURCE_DIR}/Scheduler)

add_library(${PROJECT_NAME} STATIC ${MC_SRC_FILES} ${MC_INC_FILES})
