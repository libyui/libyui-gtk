# cmake 2.6 requires this
cmake_minimum_required (VERSION 2.4)

project (yast2-gtk)

## configure


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_BINARY_DIR}/../") 

find_package (GTK2 REQUIRED gtk)
find_package (Libyui REQUIRED)
find_package (Zypp REQUIRED)

# debug while GTK2 support is shaky
message (STATUS "GTK2 includes:" ${GTK2_INCLUDE_DIRS})
message (STATUS "GTK2 libraries:" ${GTK2_LIBRARIES})

# set libyui and libzypp versions
execute_process (COMMAND pkg-config --modversion yast2-libyui COMMAND awk -F. "{ printf \"%d\", ($1 * 1000 + $2) * 1000 + $3;}" OUTPUT_VARIABLE YAST2_VERSION)
if (NOT YAST2_VERSION)
  message (FATAL_ERROR "Could not parse YaST2 (libyui) version.")
endif (NOT YAST2_VERSION)

execute_process (COMMAND pkg-config --modversion libzypp COMMAND awk -F. "{ printf \"%d\", ($1 * 1000 + $2) * 1000 + $3;}" OUTPUT_VARIABLE ZYPP_VERSION)
if (NOT ZYPP_VERSION)
  message (FATAL_ERROR "Could not parse Zypp version.")
endif (NOT ZYPP_VERSION)

set (CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/share/YaST2/data/devtools/cmake/modules" ${CMAKE_MODULE_PATH})
include (YastCommon)
set (YAST_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/YaST2)

#set (DATADIR ${CMAKE_INSTALL_PREFIX}/share/YaST2/gtk/)
set (LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/YaST2/locale)
set (THEMEDIR ${CMAKE_INSTALL_PREFIX}/share/YaST2/theme/current)

# create config.h (see config.h.cmake)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

## gcc flags

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Woverloaded-virtual -Wno-deprecated")
set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -g -Wall")
set (CMAKE_CXX_FLAGS_RELEASE   "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

## make package

set (RPMNAME "yast2-gtk")
generate_packaging (${RPMNAME} ${VERSION})

add_subdirectory (src)

