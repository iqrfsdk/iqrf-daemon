project(ServiceExample)

FIND_PACKAGE(clibdpa REQUIRED)

set(_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ServiceExample.cpp
)

set(_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ServiceExample.h
)

include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer)

add_library(${PROJECT_NAME} STATIC ${_SRC_FILES} ${_INC_FILES})
