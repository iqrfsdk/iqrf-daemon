project(iqrfapp)

set(iqrfapp_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(iqrfapp_INC_FILES
)

add_executable(${PROJECT_NAME} ${iqrfapp_SRC_FILES} ${iqrfapp_INC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} MqChannel )
else()
	target_link_libraries(${PROJECT_NAME} MqChannel pthread rt)
endif()

if (WIN32)
else()
INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)
endif()
