project(iqrf_ide_dummy)

set(iqrf_ide_dummy_SRC_FILES
	main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/udp)

add_executable(${PROJECT_NAME} ${iqrf_ide_dummy_SRC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} udp Ws2_32)
else()
	target_link_libraries(${PROJECT_NAME} udp pthread)
endif()

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
