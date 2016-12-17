project(iqrfapp)

FIND_PACKAGE(clibdpa REQUIRED)
FIND_PACKAGE(clibcdc REQUIRED)
FIND_PACKAGE(cutils REQUIRED)

set(iqrfapp_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(iqrfapp_INC_FILES
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/DpaExtension)

include_directories(${clibcdc_INCLUDE_DIRS})
include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${cutils_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${iqrfapp_SRC_FILES} ${iqrfapp_INC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} MqChannel DpaExtension dpa)
else()
	target_link_libraries(${PROJECT_NAME} MqChannel DpaExtension dpa pthread rt)
endif()

if (WIN32)
else()
INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)
endif()
