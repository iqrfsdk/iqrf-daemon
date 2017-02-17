project(MessagingController)

set(MC_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MessagingController.cpp
)

set(MC_INC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/MessagingController.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/UdpChannel)
include_directories(${CMAKE_SOURCE_DIR}/MqChannel)
include_directories(${CMAKE_SOURCE_DIR}/UdpMessaging)
include_directories(${CMAKE_SOURCE_DIR}/MqMessaging)
include_directories(${CMAKE_SOURCE_DIR}/MqttMessaging)
include_directories(${CMAKE_SOURCE_DIR}/Scheduler)
include_directories(${CMAKE_SOURCE_DIR}/DpaExtension)
include_directories(${CMAKE_SOURCE_DIR}/IqrfCdcChannel)
include_directories(${CMAKE_SOURCE_DIR}/IqrfSpiChannel)
include_directories(${CMAKE_SOURCE_DIR}/SimpleSerializer)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer)
include_directories(${CMAKE_SOURCE_DIR}/JsonSerializer/include)
include_directories(${CMAKE_SOURCE_DIR}/ClientServicePlain)
include_directories(${CMAKE_SOURCE_DIR}/ClientService)

add_library(${PROJECT_NAME} STATIC ${MC_SRC_FILES} ${MC_INC_FILES})
