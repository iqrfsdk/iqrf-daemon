project(ProtocolBridgeClientService)

FIND_PACKAGE(clibdpa REQUIRED)

set(ProtocolBridgeClientService_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ProtocolBridgeClientService.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ProtocolBridge.cpp
)

set(ProtocolBridgeClientService_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ProtocolBridgeClientService.h
	${CMAKE_CURRENT_SOURCE_DIR}/ProtocolBridge.h
)

include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer)

add_library(${PROJECT_NAME} STATIC ${ProtocolBridgeClientService_SRC_FILES} ${ProtocolBridgeClientService_INC_FILES})

SET_TARGET_PROPERTIES(
	${PROJECT_NAME} PROPERTIES
	COMPILE_FLAGS -DTHERM_SIM
)
