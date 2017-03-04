project(JsonSerializer)

set(JsonSerializer_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/JsonSerializer.cpp
)

set(JsonSerializer_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/JsonSerializer.h
)

add_library(${PROJECT_NAME} STATIC ${JsonSerializer_SRC_FILES} ${JsonSerializer_INC_FILES})
