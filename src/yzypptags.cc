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
  const char *text = 0;
	switch (group)
  {
	case PK_GROUP_ENUM_ACCESSIBILITY:
		text = _("Accessibility");
		break;
	case PK_GROUP_ENUM_ACCESSORIES:
		text = _("Accessories");
		break;
	case PK_GROUP_ENUM_EDUCATION:
		text = _("Education");
		break;
	case PK_GROUP_ENUM_GAMES:
		text = _("Games");
		break;
	case PK_GROUP_ENUM_GRAPHICS:
		text = _("Graphics");
		break;
	case PK_GROUP_ENUM_INTERNET:
		text = _("Internet");
		break;
	case PK_GROUP_ENUM_OFFICE:
		text = _("Office");
		break;
	case PK_GROUP_ENUM_OTHER:
		text = _("Other");
		break;
	case PK_GROUP_ENUM_PROGRAMMING:
		text = _("Programming");
		break;
	case PK_GROUP_ENUM_MULTIMEDIA:
		text = _("Multimedia");
		break;
	case PK_GROUP_ENUM_SYSTEM:
		text = _("System");
		break;
	case PK_GROUP_ENUM_DESKTOP_GNOME:
		text = _("GNOME desktop");
		break;
	case PK_GROUP_ENUM_DESKTOP_KDE:
		text = _("KDE desktop");
		break;
	case PK_GROUP_ENUM_DESKTOP_XFCE:
		text = _("XFCE desktop");
		break;
	case PK_GROUP_ENUM_DESKTOP_OTHER:
		text = _("Other desktops");
		break;
	case PK_GROUP_ENUM_PUBLISHING:
		text = _("Publishing");
		break;
	case PK_GROUP_ENUM_SERVERS:
		text = _("Servers");
		break;
	case PK_GROUP_ENUM_FONTS:
		text = _("Fonts");
		break;
	case PK_GROUP_ENUM_ADMIN_TOOLS:
		text = _("Admin tools");
		break;
	case PK_GROUP_ENUM_LEGACY:
		text = _("Legacy");
		break;
	case PK_GROUP_ENUM_LOCALIZATION:
		text = _("Localization");
		break;
	case PK_GROUP_ENUM_VIRTUALIZATION:
		text = _("Virtualization");
		break;
	case PK_GROUP_ENUM_SECURITY:
		text = _("Security");
		break;
	case PK_GROUP_ENUM_POWER_MANAGEMENT:
		text = _("Power management");
		break;
	case PK_GROUP_ENUM_COMMUNICATION:
		text = _("Communication");
		break;
	case PK_GROUP_ENUM_NETWORK:
		text = _("Network");
		break;
	case PK_GROUP_ENUM_MAPS:
		text = _("Maps");
		break;
	case PK_GROUP_ENUM_REPOS:
		text = _("Software sources");
		break;
      case YPKG_GROUP_ALL:
          text = _("All packages");
          break;
          
      case YPKG_GROUP_SUGGESTED:
          text = _("Suggested packages");
          break;
          
      case YPKG_GROUP_RECOMMENDED:
          text = _("Recommended packages");
          break;
          
	case PK_GROUP_ENUM_UNKNOWN:
		text = _("Unknown group");
		break;
      
	}
	return text;
}

const char *
zypp_tag_enum_to_icon (YPkgGroupEnum group)
{
  const char *text;
	switch (group)
  {
	case PK_GROUP_ENUM_ACCESSIBILITY:
		text = "package_main";
		break;
	case PK_GROUP_ENUM_ACCESSORIES:
		text = "package_applications";
		break;
	case PK_GROUP_ENUM_EDUCATION:
		text = "package_edutainment";
		break;
	case PK_GROUP_ENUM_GAMES:
		text = "package_games";
		break;
	case PK_GROUP_ENUM_GRAPHICS:
		text = "package_graphics";
		break;
	case PK_GROUP_ENUM_INTERNET:
		text = "package_network";
		break;
	case PK_GROUP_ENUM_OFFICE:
		text = "applications-office";
		break;
	case PK_GROUP_ENUM_OTHER:
		text = "package_main";
		break;
	case PK_GROUP_ENUM_PROGRAMMING:
		text = "package_development";
		break;
	case PK_GROUP_ENUM_MULTIMEDIA:
		text = "package_multimedia";
		break;
	case PK_GROUP_ENUM_SYSTEM:
		text = "applications-system";
		break;
	case PK_GROUP_ENUM_DESKTOP_GNOME:
		text = "pattern-gnome";
		break;
	case PK_GROUP_ENUM_DESKTOP_KDE:
		text = "pattern-kde";
		break;
	case PK_GROUP_ENUM_DESKTOP_XFCE:
      text = "pattern-xfce";
		break;
	case PK_GROUP_ENUM_DESKTOP_OTHER:
		text = "user-desktop";
		break;
	case PK_GROUP_ENUM_PUBLISHING:
		text = "package_main";
		break;
	case PK_GROUP_ENUM_SERVERS:
		text = "package_editors";
		break;
	case PK_GROUP_ENUM_FONTS:
		text = "package_main";
		break;
	case PK_GROUP_ENUM_ADMIN_TOOLS:
		text = "yast-sysconfig";
		break;
	case PK_GROUP_ENUM_LEGACY:
      text = "package_main";
		break;
	case PK_GROUP_ENUM_LOCALIZATION:
		text = "yast-language";
		break;
	case PK_GROUP_ENUM_VIRTUALIZATION:
		text = "yast-create-new-vm";
		break;
	case PK_GROUP_ENUM_SECURITY:
		text = "yast-security";
		break;
	case PK_GROUP_ENUM_POWER_MANAGEMENT:
		text = "package_settings_power";
		break;
	case PK_GROUP_ENUM_COMMUNICATION:
		text = "yast-modem";
		break;
	case PK_GROUP_ENUM_NETWORK:
		text = "package_network";
		break;
	case PK_GROUP_ENUM_MAPS:
		text = "package_main";
		break;
	case PK_GROUP_ENUM_REPOS:
		text = "package_main";
		break;
  case YPKG_GROUP_SUGGESTED:
      case YPKG_GROUP_RECOMMENDED:
          text = "package_edutainment_languages";
          
    break;         
	case PK_GROUP_ENUM_UNKNOWN:
  case YPKG_GROUP_ALL:
		text = "package_main";
		break;
      
	}
	return text;
}


YPkgGroupEnum
zypp_tag_convert (const std::string &groupu)
{
    // TODO Look for a faster and nice way to do this conversion

    std::string group = zypp::str::toLower(groupu);

    if (group.find ("amusements") != std::string::npos) {
        return PK_GROUP_ENUM_GAMES;
    } else if (group.find ("development") != std::string::npos) {
        return PK_GROUP_ENUM_PROGRAMMING;
    } else if (group.find ("hardware") != std::string::npos) {
        return PK_GROUP_ENUM_SYSTEM;
    } else if (group.find ("archiving") != std::string::npos 
               || group.find("clustering") != std::string::npos
               || group.find("system/monitoring") != std::string::npos
               || group.find("databases") != std::string::npos
               || group.find("system/management") != std::string::npos) {
        return PK_GROUP_ENUM_ADMIN_TOOLS;
    } else if (group.find ("graphics") != std::string::npos) {
        return PK_GROUP_ENUM_GRAPHICS;
    } else if (group.find ("multimedia") != std::string::npos) {
        return PK_GROUP_ENUM_MULTIMEDIA;
    } else if (group.find ("network") != std::string::npos) {
        return PK_GROUP_ENUM_NETWORK;
    } else if (group.find ("office") != std::string::npos 
               || group.find("text") != std::string::npos
               || group.find("editors") != std::string::npos) {
        return PK_GROUP_ENUM_OFFICE;
    } else if (group.find ("publishing") != std::string::npos) {
        return PK_GROUP_ENUM_PUBLISHING;
    } else if (group.find ("security") != std::string::npos) {
        return PK_GROUP_ENUM_SECURITY;
    } else if (group.find ("telephony") != std::string::npos) {
        return PK_GROUP_ENUM_COMMUNICATION;
    } else if (group.find ("gnome") != std::string::npos) {
        return PK_GROUP_ENUM_DESKTOP_GNOME;
    } else if (group.find ("kde") != std::string::npos) {
        return PK_GROUP_ENUM_DESKTOP_KDE;
    } else if (group.find ("xfce") != std::string::npos) {
        return PK_GROUP_ENUM_DESKTOP_XFCE;
    } else if (group.find ("gui/other") != std::string::npos) {
        return PK_GROUP_ENUM_DESKTOP_OTHER;
    } else if (group.find ("localization") != std::string::npos) {
        return PK_GROUP_ENUM_LOCALIZATION;
    } else if (group.find ("system") != std::string::npos) {
        return PK_GROUP_ENUM_SYSTEM;
    } else if (group.find ("scientific") != std::string::npos) {
        return PK_GROUP_ENUM_EDUCATION;
    }
    
    return PK_GROUP_ENUM_UNKNOWN;
}

