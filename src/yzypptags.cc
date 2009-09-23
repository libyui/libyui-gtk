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
#include "yzypptags.h"
#include <string>

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
	case PK_GROUP_ENUM_DOCUMENTATION:		return _( "Documentation"		);
	case PK_GROUP_ENUM_UTILITIES:		return _( "Utilities"		);
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
	case PK_GROUP_ENUM_DOCUMENTATION:		return( "package_documentation"			);
	case PK_GROUP_ENUM_UTILITIES:		return( "package_utilities"			);
	case PK_GROUP_ENUM_UNKNOWN:		return( "package_main"			);
	}
	return "";
}


YPkgGroupEnum
zypp_tag_convert (const std::string &groupu)
{
    std::string group (groupu);  // lower-case
    for (unsigned int i = 0; i < group.length(); i++)
    	if (group[i] >= 'A' && group[i] <= 'Z')
    		group[i] = group[i] - 'A' + 'a';

	// yast2-qt: (modified to speed up)
	if (group.compare (0, 22, "productivity/archiving") == 0) return PK_GROUP_ENUM_ADMIN_TOOLS;
	if (group.compare (0, 23, "productivity/clustering") == 0) return PK_GROUP_ENUM_ADMIN_TOOLS;
    if (group.compare (0, 22, "productivity/databases") == 0) return PK_GROUP_ENUM_ADMIN_TOOLS;
	if (group.compare (0, 17, "system/monitoring") == 0) return PK_GROUP_ENUM_ADMIN_TOOLS;
	if (group.compare (0, 17, "system/management") == 0) return PK_GROUP_ENUM_ADMIN_TOOLS;
    if (group.compare (0, 23, "productivity/publishing") == 0) return PK_GROUP_ENUM_PUBLISHING;
    if (group.compare (0, 22, "productivity/telephony") == 0) return PK_GROUP_ENUM_COMMUNICATION;
    if (group.compare (0, 19, "amusements/teaching") == 0) return PK_GROUP_ENUM_EDUCATION;
    if (group.compare (0, 17, "publishing/office") == 0) return PK_GROUP_ENUM_OFFICE;
    if (group.compare (0, 18, "producitivity/text") == 0) return PK_GROUP_ENUM_OFFICE;
    if (group.compare (0, 21, "producitivity/editors") == 0) return PK_GROUP_ENUM_OFFICE;
    if (group.compare (0, 20, "producitivity/graphics") == 0) return PK_GROUP_ENUM_GRAPHICS;
    if (group.compare (0, 10, "amusements") == 0) return PK_GROUP_ENUM_GAMES;
    if (group.compare (0, 19, "system/localization") == 0) return PK_GROUP_ENUM_LOCALIZATION;
    if (group.compare (0, 11, "development") == 0) return PK_GROUP_ENUM_PROGRAMMING;
    if (group.compare (0, 21, "producitivity/network") == 0) return PK_GROUP_ENUM_NETWORK;
    if (group.compare (0, 22, "producitivity/security") == 0) return PK_GROUP_ENUM_SECURITY;
    if (group.compare (0, 16, "system/gui/gnome") == 0) return PK_GROUP_ENUM_DESKTOP_GNOME;
    if (group.compare (0, 14, "system/gui/kde") == 0) return PK_GROUP_ENUM_DESKTOP_KDE;
    if (group.compare (0, 15, "system/gui/xfce") == 0) return PK_GROUP_ENUM_DESKTOP_XFCE;
    if (group.compare (0, 10, "system/gui") == 0) return PK_GROUP_ENUM_DESKTOP_OTHER;
    if (group.compare (0, 8, "hardware") == 0) return PK_GROUP_ENUM_SYSTEM;
    if (group.compare (0, 6, "system") == 0) return PK_GROUP_ENUM_SYSTEM;
    if (group.find ("scientific") != string::npos) return PK_GROUP_ENUM_EDUCATION;
    if (group.find ("multimedia") != string::npos) return PK_GROUP_ENUM_MULTIMEDIA;

	// our own:
    if (group.compare (0, 13, "documentation") == 0) return PK_GROUP_ENUM_DOCUMENTATION;
    if (group.compare (0, 12, "productivity") == 0) return PK_GROUP_ENUM_UTILITIES;
    return PK_GROUP_ENUM_UNKNOWN;
}

