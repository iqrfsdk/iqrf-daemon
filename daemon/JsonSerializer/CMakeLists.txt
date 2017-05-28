project(JsonSerializer)

set(JsonSerializer_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/JsonSerializer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PrfStdSen.cpp
)

set(JsonSerializer_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/JsonSerializer.h
	${CMAKE_CURRENT_SOURCE_DIR}/PrfStdSen.h
)

add_library(${PROJECT_NAME} STATIC ${JsonSerializer_SRC_FILES} ${JsonSerializer_INC_FILES})
