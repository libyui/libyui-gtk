##### PROJECTINFO for CMake

SET( BASELIB		"yui" )		# don't change this

##### MAKE ALL NEEDED CHANGES HERE #####

SET( SUBDIRS		src )
SET( PLUGINNAME		"gtk" )
SET( LIB_DEPS		GTK3 )
SET( INTERNAL_DEPS	Libyui )
SET( LIB_LINKER		)
SET( EXTRA_INCLUDES	)
SET( PROGSUBDIR		"" )
SET( URL                "http://github.com/libyui/" )
SET( SUMMARY            "Libyui - Gtk User Interface" )
SET( DESCRIPTION        "This package contains the Gtk user interface\ncomponent for libYUI.\n" )

