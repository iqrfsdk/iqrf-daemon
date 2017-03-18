# - Try to find paho
#  Once done this will define
#  PAHO_FOUND - System has paho
#  PAHO_ROOT_DIR - The paho root path
#  PAHO_INCLUDE_DIRS - The paho include directories
#  PAHO_LIBRARY_DIRS - The libraries include directories
# 
#	To satisfy this finder paho has to be installed properly
#   Download https://github.com/eclipse/paho.mqtt.c.git
#   Build in a directory outside sources e.g:
#     mkdir pahoBuild
#	  cd pahoBuild
#	  cmake -G "Visual Studio 12 2013 Win64" -DPAHO_WITH_SSL=FALSE -DPAHO_BUILD_DOCUMENTATION=FALSE -DPAHO_BUILD_SAMPLES=TRUE ../paho.mqtt.c
#	  cd ..
#	  cmake --build ./pahoBuild
#	Install with admin(WIN) privileges or sudo(LIN)
#	  cmake --build ./pahoBuild --target install 
#  Linux specific:
#    Paho is installed in /usr/local/bin and /usr/local/include
#  Windows specific:
#    Paho intallation path has to be specified in the PATH env. variable:
#    C:\Program Files\paho\bin
#    It is used by this finder and the system to find paho*.dll 

find_path(PAHO_ROOT_DIR
    include/MQTTClient.h
)

find_path(PAHO_INC_DIR
    MQTTClient.h
)

find_library(PAHO_C_LIB 
    paho-mqtt3c
)

get_filename_component(PAHO_LIB_DIR ${PAHO_C_LIB} DIRECTORY)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LOGGING_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PAHO DEFAULT_MSG PAHO_INC_DIR PAHO_C_LIB)

if (PAHO_FOUND)
    set(PAHO_INCLUDE_DIRS ${PAHO_INC_DIR} )
    set(PAHO_LIBRARY_DIRS ${PAHO_LIB_DIR} )
    set(PAHO_DEFINITIONS )
    #MESSAGE(STATUS "PAHO_DEFINITIONS: " ${PAHO_DEFINITIONS})
endif()

# Tell cmake GUIs to ignore the "local" variables.
mark_as_advanced(PAHO_INC_DIR PAHO_LIB_DIR PAHO_C_LIB)

