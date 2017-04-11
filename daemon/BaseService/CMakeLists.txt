project(BaseService)

set(BaseService_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/BaseService.cpp
)

set(BaseService_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/BaseService.h
)

add_library(${PROJECT_NAME} STATIC ${BaseService_SRC_FILES} ${BaseService_INC_FILES})
