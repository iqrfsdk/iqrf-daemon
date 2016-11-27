project(SimpleSerializer)

FIND_PACKAGE(clibdpa REQUIRED)

set(SimpleSerializer_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/SimpleSerializer.cpp
)

set(SimpleSerializer_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/SimpleSerializer.h
)

#include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${SimpleSerializer_SRC_FILES} ${SimpleSerializer_INC_FILES})
