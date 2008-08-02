AM_CFLAGS = $(GTK_CFLAGS) \
	-DY2LOG=\"ui-gtk\" \
	-DTHEMEDIR=\"${themedir}/current\" \
	-DLOCALEDIR=\"${localedir}\"
AM_CXXFLAGS = $(AM_CFLAGS)

noinst_HEADERS = $(wildcard *.h)

# test linkage for missing symbols etc.
noinst_PROGRAMS = dummy

# plugin, libtool forces 'lib' prefix
plugin_LTLIBRARIES = libpy2gtk.la

libpy2gtk_la_LDFLAGS = -version-info 2:0 -Wl,--no-undefined
libpy2gtk_la_LIBADD = -L$(plugindir) -L$(libdir) -lycp -ly2 -lyui -lm \
                      $(GTK_LIBS) $(ZYPP_LIBS) $(GTKHTML_LIBS) -ly2util
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
	YGInputField.cc \
	YGText.cc \
	YGIntField.cc \
	YGRadioButton.cc \
	YGImage.cc \
	YGFrame.cc \
	YGSelectionModel.cc \
	YGComboBox.cc \
	YGTable.cc \
	YGMenuButton.cc \
	YGBarGraph.cc \
	YGDumbTab.cc \
	YGWizard.cc \
	YGPackageSelector.cc \
	ygtkratiobox.c \
	ygtkfixed.c \
	ygtkbargraph.c \
	ygtksteps.c \
	ygtkfieldentry.c \
	ygtkmenubutton.c \
	ygtkwizard.c \
	ygtkfindentry.c \
	ygdkmngloader.c \
	ygtkimage.c \
	ygtkcellrenderertextpixbuf.c \
	ygtkscrolledwindow.c \
	ygtktogglebutton.c \
	ygtkhtmlwrap.c \
	ygtktimezonepicker.c \
	ygtkhandlebox.c \
	ygtklinklabel.c \
	yzyppwrapper.cc \
	yzypptags.cc \
	ygtkzyppwrapper.cc \
	ygtkrichtext.c
# should only append ygtkrichtext.c if USE_GTKHTML is set

dummy_SOURCES = dummy.cc
dummy_LDADD = libpy2gtk.la

check_PROGRAMS = test
TESTS = $(check_PROGRAMS)

test_SOURCES = test.cc
test_LDADD = $(top_builddir)/src/$(plugin_LTLIBRARIES)

INCLUDES = -I$(srcdir)/include -I$(includedir) -I$(includedir)/yui $(ZYPP_CFLAGS)

EXTRA_DIST = \
	icons/pkg-available-locked.xpm \
	icons/pkg-installed-upgradable.xpm \
	icons/pkg-remove-auto.xpm \
	icons/pkg-available.xpm \
	icons/pkg-installed.xpm \
	icons/pkg-remove.xpm \
	icons/pkg-install-auto.xpm \
	icons/pkg-install.xpm \
	icons/pkg-tiles-mode.xpm \
	icons/pkg-installed-locked.xpm \
	icons/pkg-list-mode.xpm \
	icons/pkg-unlocked.xpm \
	icons/pkg-installed-upgradable-locked.xpm \
	icons/pkg-locked.xpm \
	icons/harddisk.xpm \
	icons/harddisk-full.xpm \
	icons/hr.xpm \
	pkg-selector-help.h

