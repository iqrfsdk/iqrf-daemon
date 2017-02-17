project(SimpleSerializer)

set(SimpleSerializer_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/SimpleSerializer.cpp
)

set(SimpleSerializer_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/SimpleSerializer.h
)

add_library(${PROJECT_NAME} STATIC ${SimpleSerializer_SRC_FILES} ${SimpleSerializer_INC_FILES})
