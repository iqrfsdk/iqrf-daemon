#static udp library
project(udp LANGUAGES C)

#declaration of the private implementation ${CMAKE_SOURCE_DIR}/src/CDCImplPri.h
#is intentionally kept outside of interface declarations

#platform dependant parts of the private implementation 
# if (WIN32) 
	# set(CDCPlatforSpec_SRC
		# ${CMAKE_SOURCE_DIR}/src/CDCImpl_Win.cpp
	# )
# else()
	# set(CDCPlatforSpec_SRC
		# ${CMAKE_SOURCE_DIR}/src/CDCImpl_Lin.cpp
	# )
# endif()

# Specify source and header files.
set(udp_SRC_FILES
	#${CDCPlatforSpec_SRC}
	${CMAKE_CURRENT_SOURCE_DIR}/helpers.c
	${CMAKE_CURRENT_SOURCE_DIR}/IQRF_UDP.c
)

set(udp_INC_FILES
	#${CMAKE_SOURCE_DIR}/include/CDCImpl.h
	${CMAKE_CURRENT_SOURCE_DIR}/helpers.h
	${CMAKE_CURRENT_SOURCE_DIR}/IQRF_UDP.h
)

# Group the files in IDE.
source_group("include" FILES ${udp_INC_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(
	${PROJECT_NAME}
    STATIC
	${udp_SRC_FILES} ${udp_INC_FILES}
)

#TODO
#install(TARGETS ${project} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
