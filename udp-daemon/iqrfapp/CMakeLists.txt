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
include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
include_directories(${CMAKE_SOURCE_DIR}/DpaExtension)
include_directories(${CMAKE_SOURCE_DIR}/udp)

include_directories(${clibcdc_INCLUDE_DIRS})
include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${cutils_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${iqrfapp_SRC_FILES} ${iqrfapp_INC_FILES})

if (WIN32) 
	target_link_libraries(${PROJECT_NAME} DpaExtension IqrfCdcChannel dpa UdpChannel udp cdc  Ws2_32)
else()
	target_link_libraries(${PROJECT_NAME} DpaExtension IqrfCdcChannel dpa UdpChannel udp pthread)
endif()

#install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/sbin)
