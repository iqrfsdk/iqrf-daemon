project(udp-daemon)

FIND_PACKAGE(clibcdc REQUIRED)

set(udp-daemon_SRC_FILES
	main.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/udp)
include_directories(${clibcdc_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${udp-daemon_SRC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} cdc udp Ws2_32)
else()
	target_link_libraries(${PROJECT_NAME} udp pthread)
endif()

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
