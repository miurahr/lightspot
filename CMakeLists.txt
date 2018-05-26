cmake_minimum_required(VERSION 3.9)
project(lightspot C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)

##===========================
## target driver program
##===========================
set(TARGET_DRIVER lightspotd)
find_package(libusb-1.0 REQUIRED)
add_executable(${TARGET_DRIVER} src/lightspotd.c)
set_target_properties(${TARGET_DRIVER} PROPERTIES INCLUDE_DIRECTORIES ${LIBUSB_1_INCLUDE_DIRS})
set_target_properties(${TARGET_DRIVER} PROPERTIES LINK_LIBRARIES ${LIBUSB_1_LIBRARIES})

install(TARGETS ${TARGET_DRIVER}
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
