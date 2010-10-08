/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* Tags PackageKit translator */
// check the header file for information about this translator

/*
  Textdomain "gtk"
 */

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
	switch (group) {
	case YPKG_GROUP_EDUCATION:		return _( "Education"		);
	case YPKG_GROUP_GAMES:		return _( "Games"		);
	case YPKG_GROUP_GRAPHICS:		return _( "Graphics"		);
	case YPKG_GROUP_OFFICE:		return _( "Office"		);
	case YPKG_GROUP_PROGRAMMING:		return _( "Programming"		);
	case YPKG_GROUP_MULTIMEDIA:		return _( "Multimedia"		);
	case YPKG_GROUP_SYSTEM:		return _( "System"		);
	// Translators: keep translation short
	case YPKG_GROUP_DESKTOP_GNOME:	return _( "Desktop (GNOME)"	);
	// Translators: keep translation short
	case YPKG_GROUP_DESKTOP_KDE:		return _( "Desktop (KDE)"		);
	// Translators: keep translation short
	case YPKG_GROUP_DESKTOP_XFCE:	return _( "Desktop (XFCE)"	);
	// Translators: keep translation short
	case YPKG_GROUP_DESKTOP_OTHER:	return _( "Desktop (Others)"	);
	case YPKG_GROUP_PUBLISHING:		return _( "Publishing"		);
	// Translators: keep translation short
	case YPKG_GROUP_ADMIN_TOOLS:		return _( "Admin Tools"		);
	case YPKG_GROUP_LOCALIZATION:	return _( "Localization"	);
	case YPKG_GROUP_SECURITY:		return _( "Security"		);
	case YPKG_GROUP_COMMUNICATION:	return _( "Communication"	);
	case YPKG_GROUP_NETWORK:		return _( "Network"		);
	case YPKG_GROUP_DOCUMENTATION:		return _( "Documentation"		);
	case YPKG_GROUP_UTILITIES:		return _( "Utilities"		);

	case YPKG_GROUP_UNKNOWN:		return _( "Unknown"	);
	case YPKG_GROUP_SUGGESTED:		return _( "Suggested"	);
	case YPKG_GROUP_RECOMMENDED:		return _( "Recommended"	);
	// Translators: this refers to packages no longer available in any repository.
	case YPKG_GROUP_ORPHANED:		return _( "Unmaintained"	);
	case YPKG_GROUP_RECENT:		return _( "Recent"	);
	case YPKG_GROUP_MULTIVERSION:		return _( "Multiversion"	);
	case YPKG_GROUP_TOTAL: break;
	}
	return "";
}

const char *
zypp_tag_enum_to_icon (YPkgGroupEnum group)
{
	// mosts icons are from /usr/share/icons/hicolor/32x32/apps/package*
	switch (group)
  {
	case YPKG_GROUP_EDUCATION:		return( "package_edutainment"		);
	case YPKG_GROUP_GAMES:		return( "package_games"			);
	case YPKG_GROUP_GRAPHICS:		return( "package_graphics"		);
	case YPKG_GROUP_OFFICE:		return( "applications-office"	);
	case YPKG_GROUP_PROGRAMMING:		return( "package_development"		);
	case YPKG_GROUP_MULTIMEDIA:		return( "package_multimedia"		);
	case YPKG_GROUP_SYSTEM:		return( "applications-system"			);
	case YPKG_GROUP_DESKTOP_GNOME:	return( "pattern-gnome"			);
	case YPKG_GROUP_DESKTOP_KDE:		return( "pattern-kde"			);
	case YPKG_GROUP_DESKTOP_XFCE:	return( "pattern-xfce"	);
	case YPKG_GROUP_DESKTOP_OTHER:	return( "user-desktop"	);
	case YPKG_GROUP_PUBLISHING:		return( "package_office_projectmanagement"			);
	case YPKG_GROUP_ADMIN_TOOLS:		return( "yast-sysconfig"		);
	case YPKG_GROUP_LOCALIZATION:	return( "yast-language"			);
	case YPKG_GROUP_SECURITY:		return( "yast-security"			);
	case YPKG_GROUP_COMMUNICATION:	return( "yast-modem"			);
	case YPKG_GROUP_NETWORK:		return( "package_network"		);
	case YPKG_GROUP_DOCUMENTATION:		return( "package_documentation"			);
	case YPKG_GROUP_UTILITIES:		return( "package_utilities"			);

	case YPKG_GROUP_SUGGESTED:		return( "gtk-about" );
	case YPKG_GROUP_RECOMMENDED:		return( "gtk-about" );
	case YPKG_GROUP_ORPHANED:		return( "gtk-missing-image" );
	case YPKG_GROUP_RECENT:		return( "gtk-new"			);
	case YPKG_GROUP_MULTIVERSION:		return( "gtk-dnd-multiple" );
	case YPKG_GROUP_UNKNOWN:		return( "package_main"			);

	case YPKG_GROUP_TOTAL: break;
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
	if (group.compare (0, 22, "productivity/archiving") == 0) return YPKG_GROUP_ADMIN_TOOLS;
	if (group.compare (0, 23, "productivity/clustering") == 0) return YPKG_GROUP_ADMIN_TOOLS;
    if (group.compare (0, 22, "productivity/databases") == 0) return YPKG_GROUP_ADMIN_TOOLS;
	if (group.compare (0, 17, "system/monitoring") == 0) return YPKG_GROUP_ADMIN_TOOLS;
	if (group.compare (0, 17, "system/management") == 0) return YPKG_GROUP_ADMIN_TOOLS;
    if (group.compare (0, 23, "productivity/publishing") == 0) return YPKG_GROUP_PUBLISHING;
    if (group.compare (0, 22, "productivity/telephony") == 0) return YPKG_GROUP_COMMUNICATION;
    if (group.compare (0, 19, "amusements/teaching") == 0) return YPKG_GROUP_EDUCATION;
    if (group.compare (0, 17, "publishing/office") == 0) return YPKG_GROUP_OFFICE;
    if (group.compare (0, 17, "productivity/text") == 0) return YPKG_GROUP_OFFICE;
    if (group.compare (0, 20, "productivity/editors") == 0) return YPKG_GROUP_OFFICE;
    if (group.compare (0, 21, "productivity/graphics") == 0) return YPKG_GROUP_GRAPHICS;
    if (group.compare (0, 10, "amusements") == 0) return YPKG_GROUP_GAMES;
    if (group.compare (0, 19, "system/localization") == 0) return YPKG_GROUP_LOCALIZATION;
    if (group.compare (0, 11, "development") == 0) return YPKG_GROUP_PROGRAMMING;
    if (group.compare (0, 20, "productivity/network") == 0) return YPKG_GROUP_NETWORK;
    if (group.compare (0, 21, "productivity/security") == 0) return YPKG_GROUP_SECURITY;
    if (group.compare (0, 16, "system/gui/gnome") == 0) return YPKG_GROUP_DESKTOP_GNOME;
    if (group.compare (0, 14, "system/gui/kde") == 0) return YPKG_GROUP_DESKTOP_KDE;
    if (group.compare (0, 15, "system/gui/xfce") == 0) return YPKG_GROUP_DESKTOP_XFCE;
    if (group.compare (0, 10, "system/gui") == 0) return YPKG_GROUP_DESKTOP_OTHER;
    if (group.compare (0, 8, "hardware") == 0) return YPKG_GROUP_SYSTEM;
    if (group.compare (0, 6, "system") == 0) return YPKG_GROUP_SYSTEM;
    if (group.find ("scientific") != string::npos) return YPKG_GROUP_EDUCATION;
    if (group.find ("multimedia") != string::npos) return YPKG_GROUP_MULTIMEDIA;

	// our own:
    if (group.compare (0, 13, "documentation") == 0) return YPKG_GROUP_DOCUMENTATION;
    if (group.compare (0, 12, "productivity") == 0) return YPKG_GROUP_UTILITIES;
    return YPKG_GROUP_UNKNOWN;
}

