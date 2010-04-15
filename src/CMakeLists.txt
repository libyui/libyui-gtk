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
	ygtkbargraph.c
	ygtkfieldentry.c
	ygtkhtmlwrap.c
	ygtkmenubutton.c
	ygtklinklabel.c
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

add_subdirectory (pkg)

## test:

link_directories (${CMAKE_BINARY_DIR}/src)
add_executable (test test.cc)
add_dependencies (test py2gtk)
target_link_libraries (test libpy2gtk.so)
target_link_libraries (test ${GTK2_LIBRARIES})
target_link_libraries (test ${LIBYUI_LIBRARY})

