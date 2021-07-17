# CMakeLists.txt for libyui-gtk/pkgconfig
#
# Support for pkg-config:
#
# Generate a libyui-gtk.pc file from libyui-gtk.pc.in and install it to
# /usr/lib64/pkgconfig.
#
# A .pc file specifies how to use a development package, in particular linker
# flags (-lyui-gtk), compiler flags (including include directories) and paths.
# See man pkg-config.
#
# This .pc file is intended for extensions of the Gtk UI plug-in, not for
# applications using libyui.

include( ../VERSION.cmake )
include( GNUInstallDirs )       # set CMAKE_INSTALL_LIBDIR

FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(YUI REQUIRED libyui)
pkg_get_variable(YUI_SO_VERSION libyui soversion)
pkg_get_variable(YUI_SO_MAJOR libyui soversion_major)
pkg_get_variable(YUI_SO_MINOR libyui soversion_minor)
pkg_get_variable(YUI_SO_PATCH libyui soversion_patch)

##### This is needed to be set for the libyui core
SET( SONAME ${YUI_SO_VERSION} )
SET( SONAME_MAJOR ${YUI_SO_MAJOR} )
SET( SONAME_MINOR ${YUI_SO_MINOR} )
SET( SONAME_PATCH ${YUI_SO_PATCH} )

# See https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
# CMAKE_INSTALL_FULL_<dir>
# The absolute path generated from the corresponding CMAKE_INSTALL_<dir> value.
# If the value is not already an absolute path, an absolute path is constructed
# typically by prepending the value of the CMAKE_INSTALL_PREFIX variable.
# So that covers also cmake directive -DCMAKE_INSTALL_LIBDIR=/usr/lib64 with full
# pathname.
# CMAKE_INSTALL_FULL_LIBDIR is used to install libyui-gtk.pc and inside it
# to set "libdir" and "plugindir" pkg-config variables
set( PKGCONFIG_INSTALL_DIR ${DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig )

# Generate libyui-gtk.pc where some CMake variables are expanded from libyui-gtk.pc.in,
# but only expand @VARIABLE@, not ${VARIABLE}
configure_file( libyui-gtk.pc.in libyui-gtk.pc @ONLY )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/libyui-gtk.pc DESTINATION ${PKGCONFIG_INSTALL_DIR} )
