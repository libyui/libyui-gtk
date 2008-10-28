/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/*
  Textdomain "yast2-gtk"
 */

/* Tags PackageKit translator */
// check the header file for information about this translator

#include <config.h>
#include "YGi18n.h"
#define YUILogComponent "gtk-pkg"
#include <YUILog.h>
#include <zypp/sat/LookupAttr.h>
#include "yzypptags.h"

/**
 * translations taken from packagekit
 **/
const char *
zypp_tag_group_enum_to_localised_text (YPkgGroupEnum group)
{
	switch (group)
  {
	case PK_GROUP_ENUM_ACCESSIBILITY:	return _( "Accessibility"	);
	case PK_GROUP_ENUM_ACCESSORIES:		return _( "Accessories"		);
	case PK_GROUP_ENUM_EDUCATION:		return _( "Education"		);
	case PK_GROUP_ENUM_GAMES:		return _( "Games"		);
	case PK_GROUP_ENUM_GRAPHICS:		return _( "Graphics"		);
	case PK_GROUP_ENUM_INTERNET:		return _( "Internet"		);
	case PK_GROUP_ENUM_OFFICE:		return _( "Office"		);
	case PK_GROUP_ENUM_OTHER:		return _( "Other"		);
	case PK_GROUP_ENUM_PROGRAMMING:		return _( "Programming"		);
	case PK_GROUP_ENUM_MULTIMEDIA:		return _( "Multimedia"		);
	case PK_GROUP_ENUM_SYSTEM:		return _( "System"		);
	case PK_GROUP_ENUM_DESKTOP_GNOME:	return _( "GNOME Desktop"	);
	case PK_GROUP_ENUM_DESKTOP_KDE:		return _( "KDE Desktop"		);
	case PK_GROUP_ENUM_DESKTOP_XFCE:	return _( "XFCE Desktop"	);
	case PK_GROUP_ENUM_DESKTOP_OTHER:	return _( "Other Desktops"	);
	case PK_GROUP_ENUM_PUBLISHING:		return _( "Publishing"		);
	case PK_GROUP_ENUM_SERVERS:		return _( "Servers"		);
	case PK_GROUP_ENUM_FONTS:		return _( "Fonts"		);
	case PK_GROUP_ENUM_ADMIN_TOOLS:		return _( "Admin Tools"		);
	case PK_GROUP_ENUM_LEGACY:		return _( "Legacy"		);
	case PK_GROUP_ENUM_LOCALIZATION:	return _( "Localization"	);
	case PK_GROUP_ENUM_VIRTUALIZATION:	return _( "Virtualization"	);
	case PK_GROUP_ENUM_SECURITY:		return _( "Security"		);
	case PK_GROUP_ENUM_POWER_MANAGEMENT:	return _( "Power Management"	);
	case PK_GROUP_ENUM_COMMUNICATION:	return _( "Communication"	);
	case PK_GROUP_ENUM_NETWORK:		return _( "Network"		);
	case PK_GROUP_ENUM_MAPS:		return _( "Maps"		);
	case PK_GROUP_ENUM_REPOS:		return _( "Software Sources"	);
	case PK_GROUP_ENUM_UNKNOWN:		return _( "Unknown Group"	);
	}
	return _("Unknown Group");
}

const char *
zypp_tag_enum_to_icon (YPkgGroupEnum group)
{
	// NOTE: some icons are customized (bug 404818)
	switch (group)
  {
	case PK_GROUP_ENUM_ACCESSIBILITY:	return( "package_main"			);
	case PK_GROUP_ENUM_ACCESSORIES:		return( "package_applications"		);
	case PK_GROUP_ENUM_EDUCATION:		return( "package_edutainment"		);
	case PK_GROUP_ENUM_GAMES:		return( "package_games"			);
	case PK_GROUP_ENUM_GRAPHICS:		return( "package_graphics"		);
	case PK_GROUP_ENUM_INTERNET:		return( "package_network"		);
	case PK_GROUP_ENUM_OFFICE:		return( "applications-office"	);
	case PK_GROUP_ENUM_OTHER:		return( "package_main"			);
	case PK_GROUP_ENUM_PROGRAMMING:		return( "package_development"		);
	case PK_GROUP_ENUM_MULTIMEDIA:		return( "package_multimedia"		);
	case PK_GROUP_ENUM_SYSTEM:		return( "applications-system"			);
	case PK_GROUP_ENUM_DESKTOP_GNOME:	return( "pattern-gnome"			);
	case PK_GROUP_ENUM_DESKTOP_KDE:		return( "pattern-kde"			);
	case PK_GROUP_ENUM_DESKTOP_XFCE:	return( "pattern-xfce"	);
	case PK_GROUP_ENUM_DESKTOP_OTHER:	return( "user-desktop"	);
	case PK_GROUP_ENUM_PUBLISHING:		return( "package_main"			);
	case PK_GROUP_ENUM_SERVERS:		return( "package_editors"		);
	case PK_GROUP_ENUM_FONTS:		return( "package_main"			);
	case PK_GROUP_ENUM_ADMIN_TOOLS:		return( "yast-sysconfig"		);
	case PK_GROUP_ENUM_LEGACY:		return( "package_main"			);
	case PK_GROUP_ENUM_LOCALIZATION:	return( "yast-language"			);
	case PK_GROUP_ENUM_VIRTUALIZATION:	return( "yast-create-new-vm"		);
	case PK_GROUP_ENUM_SECURITY:		return( "yast-security"			);
	case PK_GROUP_ENUM_POWER_MANAGEMENT:	return( "package_settings_power"	);
	case PK_GROUP_ENUM_COMMUNICATION:	return( "yast-modem"			);
	case PK_GROUP_ENUM_NETWORK:		return( "package_network"		);
	case PK_GROUP_ENUM_MAPS:		return( "package_main"			);
	case PK_GROUP_ENUM_REPOS:		return( "package_main"			);
	case PK_GROUP_ENUM_UNKNOWN:		return( "package_main"			);
	}
	return "";
}


YPkgGroupEnum
zypp_tag_convert (const std::string &groupu)
{
    std::string group = zypp::str::toLower(groupu);

    if ( group.find( "amusements/teaching"	) != string::npos ) return PK_GROUP_ENUM_EDUCATION;
    if ( group.find( "amusements"		) != string::npos ) return PK_GROUP_ENUM_GAMES;
    if ( group.find( "development"		) != string::npos ) return PK_GROUP_ENUM_PROGRAMMING;
    if ( group.find( "hardware"			) != string::npos ) return PK_GROUP_ENUM_SYSTEM;
    if ( group.find( "archiving"		) != string::npos ) return PK_GROUP_ENUM_ADMIN_TOOLS;
    if ( group.find( "clustering"		) != string::npos ) return PK_GROUP_ENUM_ADMIN_TOOLS;
    if ( group.find( "system/monitoring"	) != string::npos ) return PK_GROUP_ENUM_ADMIN_TOOLS;
    if ( group.find( "databases"		) != string::npos ) return PK_GROUP_ENUM_ADMIN_TOOLS;
    if ( group.find( "system/management"	) != string::npos ) return PK_GROUP_ENUM_ADMIN_TOOLS;
    if ( group.find( "graphics"			) != string::npos ) return PK_GROUP_ENUM_GRAPHICS;
    if ( group.find( "multimedia"		) != string::npos ) return PK_GROUP_ENUM_MULTIMEDIA;
    if ( group.find( "network"			) != string::npos ) return PK_GROUP_ENUM_NETWORK;
    if ( group.find( "office"			) != string::npos ) return PK_GROUP_ENUM_OFFICE;
    if ( group.find( "text"			) != string::npos ) return PK_GROUP_ENUM_OFFICE;
    if ( group.find( "editors"			) != string::npos ) return PK_GROUP_ENUM_OFFICE;
    if ( group.find( "publishing"		) != string::npos ) return PK_GROUP_ENUM_PUBLISHING;
    if ( group.find( "security"			) != string::npos ) return PK_GROUP_ENUM_SECURITY;
    if ( group.find( "telephony"		) != string::npos ) return PK_GROUP_ENUM_COMMUNICATION;
    if ( group.find( "gnome"			) != string::npos ) return PK_GROUP_ENUM_DESKTOP_GNOME;
    if ( group.find( "kde"			) != string::npos ) return PK_GROUP_ENUM_DESKTOP_KDE;
    if ( group.find( "xfce"			) != string::npos ) return PK_GROUP_ENUM_DESKTOP_XFCE;
    if ( group.find( "gui/other"		) != string::npos ) return PK_GROUP_ENUM_DESKTOP_OTHER;
    if ( group.find( "localization"		) != string::npos ) return PK_GROUP_ENUM_LOCALIZATION;
    if ( group.find( "system"			) != string::npos ) return PK_GROUP_ENUM_SYSTEM;
    if ( group.find( "scientific"		) != string::npos ) return PK_GROUP_ENUM_EDUCATION;

    return PK_GROUP_ENUM_UNKNOWN;
}

