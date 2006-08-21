AM_CFLAGS = $(GTK_CFLAGS) \
	-DTHEMEDIR=\"${themedir}/current\"
AM_CXXFLAGS = $(GTK_CFLAGS) \
	-DY2LOG=\"ui-gtk\" \
	-DTHEMEDIR=\"${themedir}/current\"

noinst_HEADERS = $(wildcard *.h)

# test linkage for missing symbols etc.
noinst_PROGRAMS = dummy

# plugin, libtool forces 'lib' prefix
plugin_LTLIBRARIES = libpy2gtk.la

libpy2gtk_la_LDFLAGS = -version-info 2:0
libpy2gtk_la_LIBADD = -lyui -lycp -ly2 $(GTK_LIBS) $(ZYPP_LIBS)
libpy2gtk_la_SOURCES = \
	Y2CCGtk.cc \
	YGUtils.cc \
	YGUI.cc \
	YGDialog.cc \
	YGLayout.cc \
	YGWidget.cc \
	YGLabel.cc \
	YGPushButton.cc \
	YGProgressBar.cc \
	YGTextEntry.cc \
	YGMultiLineEdit.cc \
	YGRichText.cc \
	YGIntField.cc \
	YGRadioButton.cc \
	YGImage.cc \
	YGFrame.cc \
	YGComboBox.cc \
	YGTable.cc \
	YGTree.cc \
	YGMenuButton.cc \
	YGBarGraph.cc \
	YGMultiProgressMeter.cc \
	YGDumbTab.cc \
	YGWizard.cc \
	YGTime.cc \
	YGPackageSelector.cc \
	\
	ygtkratiobox.c \
	ygtkbargraph.c \
	ygtkrichtext.c \
	ygtksteps.c \
	ygtkfieldentry.c \
	ygtkmenubutton.c \
	ygtkwizard.c

dummy_SOURCES = dummy.cc
dummy_LDADD = libpy2gtk.la

check_PROGRAMS = test
TESTS = $(check_PROGRAMS)

test_SOURCES = test.cc
test_LDADD = $(top_builddir)/src/$(plugin_LTLIBRARIES)

INCLUDES = -I$(srcdir)/include -I$(includedir) -I$(includedir)/yui
