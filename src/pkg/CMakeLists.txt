## Makefile.am

set (gtk_pkg_yast_plugin_SRCS
	pkg-selector-help.cc
	YGPackageSelector.cc
	YGPackageSelectorPluginImpl.cc
	ygtkpkgdetailview.cc
	ygtkpkghistorydialog.cc
	ygtkpkglistview.cc
	ygtkpkgfilterview.cc
	ygtkpkglanguageview.cc
	ygtkpkgmenubar.cc
	ygtkpkgpatternview.cc
	ygtkpkgproductdialog.cc
	ygtkpkgquerycombo.cc
	ygtkpkgrpmgroupsview.cc
	ygtkpkgsearchentry.cc
	ygtkpkgstatusbar.cc
	ygtkpkgundolist.cc
	yzypptags.cc
	yzyppwrapper.cc
	#ygtkpkgvestigialdialog.cc

	ygtkcellrendererbutton.c
	ygtkcellrenderersidebutton.c
	ygtkcellrenderertext.c
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
target_link_libraries (py2gtk_pkg ${GTK3_LIBRARIES})
target_link_libraries (py2gtk_pkg ${LIBYUI_LIBRARY})
target_link_libraries (py2gtk_pkg ${ZYPP_LIBRARY})
set_target_properties (py2gtk_pkg PROPERTIES SOVERSION 2)
#set_target_properties (py2gtk_pkg PROPERTIES LINK_FLAGS "--no-undefined -Wl,-rpath -Wl,${UI_PLUGIN_DIR}")
set_target_properties (py2gtk_pkg PROPERTIES LINK_FLAGS "-Wl,-rpath -Wl,${UI_PLUGIN_DIR}")

## install:

install (TARGETS py2gtk_pkg LIBRARY DESTINATION ${UI_PLUGIN_DIR})

