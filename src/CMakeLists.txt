## Makefile.am

set (gtk_yast_plugin_SRCS
	YGBarGraph.cc
	YGComboBox.cc
	YGDialog.cc
	YGDumbTab.cc
	YGFrame.cc
	YGInputField.cc
	YGIntField.cc
	YGImage.cc
	YGMenuButton.cc
	YGLabel.cc
	YGLayout.cc
	YGPackageSelectorPluginStub.cc
	YGPushButton.cc
	YGProgressBar.cc
	YGRadioButton.cc
	YGSelectionModel.cc
	YGTable.cc
	YGText.cc
	YGUtils.cc
	YGUI.cc
	YGWidget.cc
	YGWizard.cc
	ygdkmngloader.c
	ygtkcellrenderertextpixbuf.c
	ygtkbargraph.c
	ygtkfieldentry.c
	ygtkhtmlwrap.c
	ygtkmenubutton.c
	ygtklinklabel.c
	ygtkfindentry.c
	ygtkfixed.c
	ygtkimage.c
	ygtkratiobox.c
	ygtkrichtext.c
	ygtksteps.c
	ygtktextview.c
	ygtktimezonepicker.c
	ygtktreeview.c
	ygtkwizard.c
	)

# if you tell cmake to install the icons/ directory
# it will make room for the .svn dir as well...
set (gtk_yast_plugin_DATA
	icons/harddisk.png
	icons/harddisk-full.png
	icons/pkg-available.png
	icons/pkg-available-locked.png
	icons/pkg-installed.png
	icons/pkg-install.png
	icons/pkg-install-auto.png
	icons/pkg-installed-locked.png
	icons/pkg-installed-upgradable.png
	icons/pkg-installed-upgradable-locked.png
	icons/pkg-remove.png
	icons/pkg-remove-auto.png
	icons/pkg-locked.png
	icons/pkg-unlocked.png
	icons/pkg-upgrade.png
	)

## includes:

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${GTK2_INCLUDE_DIRS})
include_directories (${LIBYUI_INCLUDE_DIR})

## target:

add_library (py2gtk SHARED ${gtk_yast_plugin_SRCS})

## libraries:

target_link_libraries (py2gtk ${GTK2_LIBRARIES})
target_link_libraries (py2gtk ${LIBYUI_LIBRARY})
set_target_properties (py2gtk PROPERTIES SOVERSION 2)
set_target_properties (py2gtk PROPERTIES LINK_FLAGS "--no-undefined")

## install:

install (TARGETS py2gtk LIBRARY DESTINATION ${UI_PLUGIN_DIR})
install (FILES ${gtk_yast_plugin_DATA} DESTINATION ${DATADIR})

add_subdirectory (pkg)

## test:

link_directories (${CMAKE_BINARY_DIR}/src)
add_executable (test test.cc)
target_link_libraries (test libpy2gtk.so)
target_link_libraries (test ${GTK2_LIBRARIES})
target_link_libraries (test ${LIBYUI_LIBRARY})

