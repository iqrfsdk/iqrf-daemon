project(JsonCoding)

FIND_PACKAGE(clibdpa REQUIRED)

set(JsonCoding_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/SimpleSerializer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/JsonCoding.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test.json
)

set(JsonCoding_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/SimpleSerializer.h
	${CMAKE_CURRENT_SOURCE_DIR}/JsonCoding.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${clibdpa_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${JsonCoding_SRC_FILES} ${JsonCoding_INC_FILES})

install(
    FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/test.json
    DESTINATION ${PROJECT_BINARY_DIR}/bin)
