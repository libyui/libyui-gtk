##### RPMINFO for CMake
#####
##### THIS IS THE INFO REQUIRED FOR SPEC-FILE GENERATION

SET( SPEC_URL			"http://github.com/libyui/libyui-gtk" )							# the URL of the project

SET( SPEC_SUMMARY		"GTK frontend for libyui" )										# some brief summary

SET( SPEC_DESCRIPTION		"This is the user inferface frontend that provides GTK UI for libyui\n" )										# the description to be used, end each line with "\n" for line-breaks

SET( SPEC_BuildRequires		"cmake >= 2.8" "gcc-c++ blocxx-devel" "gtk3-devel" "gdk-pixbuf-devel" "libyui-devel >= 2.18.8" "libzypp-devel >= 6.3.0") 
# the BuildRequires every single pkg wrapped with "" and speparated with spaces

SET( SPEC_Conflicts		"" )										# the Conflicts every single pkg wrapped with "" and speparated with spaces

SET( SPEC_Provides		"yast2_ui yast2-ui-gtk" "yast2_ui_pkg" )										# the Provides every single pkg wrapped with "" and speparated with spaces



#Requires:       gtk3
#Requires:       yast2-libyui >= 2.18.8
#Requires:       yast2_theme >= 2.16.1
#Requires:       yast2-branding
#Requires:           libzypp >= 6.3.0




SET( SPEC_Obsoletes		"" )										# the Obsoletes every single pkg wrapped with "" and speparated with spaces

SET( SPEC_DEVEL_Requires	"@PROJECTNAME@@@varpfx@_SONAME_MAJOR@ = %{version}" )				# the Requires for the -devel pkg every single pkg wrapped with "" and speparated with spaces

SET( SPEC_DEVEL_Provides	"pkgconfig(@PROJECTNAME@) = %{version}" )					# the Provides for the -devel pkg

SET( SPEC_DEVEL_DOCS		"%doc %{_docdir}/@PROJECTNAME@" )						# set this, if there are examples to include in -devel-pkg
