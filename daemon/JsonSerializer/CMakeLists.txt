project(JsonSerializer)

FIND_PACKAGE(clibdpa REQUIRED)

set(JsonSerializer_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/JsonSerializer.cpp
)

set(JsonSerializer_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/JsonSerializer.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${JsonSerializer_SRC_FILES} ${JsonSerializer_INC_FILES})
