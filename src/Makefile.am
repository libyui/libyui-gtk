AM_CXXFLAGS = -DY2LOG=\"ui-gtk\" $(GTK_CFLAGS)

noinst_HEADERS = $(wildcard *.h)

# test linkage for missing symbols etc.
noinst_PROGRAMS = dummy

# plugin, libtool forces 'lib' prefix
plugin_LTLIBRARIES = libpy2gtk.la

libpy2gtk_la_LDFLAGS = -version-info 2:0
libpy2gtk_la_LIBADD = -lyui -lycp -ly2 $(GTK_LIBS)
libpy2gtk_la_SOURCES = \
	Y2CCGtk.cc \
	YGUI.cc \
	YGDialog.cc \
	YGSplit.cc \
	YGLabel.cc \
	YGPushButton.cc \
	YGReplacePoint.cc \
	YGSpacing.cc \
	YGAlignment.cc \
	YGWidget.cc \
	YGEmpty.cc \
	YGProgressBar.cc \
	YGTextEntry.cc \
	YGRichText.cc \
	YGRadioButton.cc \
	YGImage.cc \
	YGFrame.cc \
	YGComboBox.cc \
	YGSelectionBox.cc \
	YGLabeledWidget.cc \
	YGUtils.cc \
	YGTable.cc

dummy_SOURCES = dummy.cc
dummy_LDADD = libpy2gtk.la

INCLUDES = -I$(srcdir)/include -I$(includedir) -I$(includedir)/yui
