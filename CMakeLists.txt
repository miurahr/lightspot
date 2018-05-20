cmake_minimum_required(VERSION 3.9)
project(lightspot C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

##===========================
## target driver program
##===========================
set(TARGET_DRIVER lightspotd)
find_package(HIDAPI REQUIRED)
add_executable(${TARGET_DRIVER} src/lightspotd.c)
set_target_properties(${TARGET_DRIVER} PROPERTIES INCLUDE_DIRECTORIES ${HIDAPI_INCLUDE_DIR})
set_target_properties(${TARGET_DRIVER} PROPERTIES LINK_LIBRARIES ${HIDAPI_LIBRARIES})

install(TARGETS ${TARGET_DRIVER}
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
