DATADIR=/usr/share/YaST2/gtk/

AM_CFLAGS = $(GTK_CFLAGS) \
	-DY2LOG=\"ui-gtk\" \
	-DTHEMEDIR=\"${themedir}/current\" \
	-DLOCALEDIR=\"${localedir}\" \
	-DDATADIR=\"${DATADIR}\"
AM_CXXFLAGS = $(AM_CFLAGS)

noinst_HEADERS = $(wildcard *.h)

# test linkage for missing symbols etc.
noinst_PROGRAMS = dummy

# plugin, libtool forces 'lib' prefix
plugin_LTLIBRARIES = libpy2gtk.la

libpy2gtk_la_LDFLAGS = -version-info 2:0 -Wl,--no-undefined
libpy2gtk_la_LIBADD = -L$(plugindir) -L$(libdir) -lycp -ly2 -lyui -lm \
		      $(GTK_LIBS) $(ZYPP_LIBS) -ly2util
if WITH_WEBKIT
    libpy2gtk_la_LIBADD += $(WEBKIT_LIBS)
endif
if WITH_GTKHTML
    libpy2gtk_la_LIBADD += $(GTKHTML_LIBS)
endif

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
	ygtktreeview.c \
	ygtktogglebutton.c \
	ygtkhtmlwrap.c \
	ygtktimezonepicker.c \
	ygtkhandlebox.c \
	ygtklinklabel.c \
	ygtktooltip.c \
	ygtktextview.c \
	ygtkrichtext.c \
	yzyppwrapper.cc \
	yzypptags.cc \
	ygtkzyppwrapper.cc
# should (maybe?) only append ygtkrichtext.c if WITH_GTKHTML or WEBKIT isn't set

dummy_SOURCES = dummy.cc
dummy_LDADD = libpy2gtk.la

check_PROGRAMS = test
TESTS = $(check_PROGRAMS)

test_SOURCES = test.cc
test_LDADD = $(top_builddir)/src/$(plugin_LTLIBRARIES)

INCLUDES = -I$(srcdir)/include -I$(includedir) -I$(includedir)/yui $(ZYPP_CFLAGS) $(GTKHTML_CFLAGS) $(WEBKIT_CFLAGS)

sharedir = $(DATADIR)
dist_share_DATA = \
	icons/pkg-available-locked.png \
	icons/pkg-installed-upgradable.png \
	icons/pkg-remove-auto.png \
	icons/pkg-available.png \
	icons/pkg-installed.png \
	icons/pkg-remove.png \
	icons/pkg-install.png \
	icons/pkg-install-auto.png \
	icons/pkg-upgrade.png \
	icons/pkg-installed-locked.png \
	icons/pkg-unlocked.png \
	icons/pkg-installed-upgradable-locked.png \
	icons/pkg-locked.png \
	icons/pkg-tiles-mode.xpm \
	icons/pkg-list-mode.xpm \
	icons/harddisk.png \
	icons/harddisk-full.png
EXTRA_DIST = $(dist_share_DATA) \
	pkg-selector-help.h \
	hr.xpm

