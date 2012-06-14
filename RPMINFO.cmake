##### RPMINFO for CMake
#####
##### THIS IS THE INFO REQUIRED FOR SPEC-FILE GENERATION

SET( SPEC_URL			"http://github.com/libyui/libyui-gtk" )
SET( SPEC_SUMMARY		"GTK frontend for libyui" )
SET( SPEC_DESCRIPTION		"This is the user inferface frontend that provides GTK UI for libyui\n" )
SET( SPEC_BuildRequires		"cmake >= 2.8" "gcc-c++" "gtk3-devel" "gdk-pixbuf-devel" "libyui-devel" ) 
SET( SPEC_Conflicts		"" )
SET( SPEC_Provides		"yast2_ui" "yast2-ui-gtk" )
SET( SPEC_Obsoletes		"" )
SET( SPEC_DEVEL_Requires	"@PROJECTNAME@@@varpfx@_SONAME_MAJOR@ = %{version}" )
SET( SPEC_DEVEL_Provides	"pkgconfig(@PROJECTNAME@) = %{version}" )
SET( SPEC_DEVEL_DOCS		"%doc %{_docdir}/@PROJECTNAME@" )
