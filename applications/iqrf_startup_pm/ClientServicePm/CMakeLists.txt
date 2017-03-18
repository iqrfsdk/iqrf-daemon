project(ClientServicePm)

FIND_PACKAGE(clibdpa REQUIRED)

set(ClientServicePm_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ClientServicePm.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PrfPulseMeter.cpp
)

set(ClientServicePm_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ClientServicePm.h
	${CMAKE_CURRENT_SOURCE_DIR}/PrfPulseMeter.h
)

include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer)

add_library(${PROJECT_NAME} STATIC ${ClientServicePm_SRC_FILES} ${ClientServicePm_INC_FILES})

SET_TARGET_PROPERTIES(
	${PROJECT_NAME} PROPERTIES
	COMPILE_FLAGS -DTHERM_SIM
)
