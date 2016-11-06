set(_ipath ${PROJECT_SOURCE_DIR}/include)
FILE(GLOB ALL_HEADERS ${_ipath}/*.h ${_ipath}/*.hpp)
add_custom_target(all_include SOURCES ${ALL_HEADERS})
