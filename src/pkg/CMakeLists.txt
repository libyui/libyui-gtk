## Makefile.am

set (gtk_pkg_yast_plugin_SRCS
	YGPackageSelectorPluginImpl.cc
	YGPackageSelector.cc
	ygtkpkgquerycombo.cc
	ygtkpkglistview.cc
	ygtkpkgdetailview.cc
	ygtkpkgsearchentry.cc
	ygtkpkgfilterview.cc
	ygtkpkgrpmgroupsview.cc
	ygtkpkgpatternview.cc
	ygtkpkglanguageview.cc
	ygtkpkgundolist.cc
	ygtkpkgmenubar.cc
	ygtkpkgstatusbar.cc
	ygtkpkgproductdialog.cc
	yzyppwrapper.cc
	yzypptags.cc

	ygtkcellrendererbutton.c
	ygtkcellrenderersidebutton.c
	ygtktreemodel.cc
	)

## includes:

include_directories (${ZYPP_INCLUDE_DIR})
# we want libpy2gtk_pkg.so to link to libpy2gtk.so before install
# and then to find it when it is installed and on its own
link_directories (${CMAKE_BINARY_DIR}/src)
link_directories (${UI_PLUGIN_DIR})

## target:

add_library (py2gtk_pkg SHARED ${gtk_pkg_yast_plugin_SRCS})
add_dependencies (py2gtk_pkg py2gtk)

## libraries:

target_link_libraries (py2gtk_pkg libpy2gtk.so)
target_link_libraries (py2gtk_pkg ${GTK2_LIBRARIES})
target_link_libraries (py2gtk_pkg ${LIBYUI_LIBRARY})
target_link_libraries (py2gtk_pkg ${ZYPP_LIBRARY})
set_target_properties (py2gtk_pkg PROPERTIES SOVERSION 2)
set_target_properties (py2gtk_pkg PROPERTIES LINK_FLAGS "--no-undefined -Wl,-rpath -Wl,${UI_PLUGIN_DIR}")

## install:

install (TARGETS py2gtk_pkg LIBRARY DESTINATION ${UI_PLUGIN_DIR})

