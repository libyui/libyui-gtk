/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Converts RPM to PackageKit-like terminology.
   Code from yast-qt-pkg.
*/

typedef enum {
  /* PackageKit values  */
	PK_GROUP_ENUM_ACCESSIBILITY,
	PK_GROUP_ENUM_ACCESSORIES,
	PK_GROUP_ENUM_EDUCATION,
	PK_GROUP_ENUM_GAMES,
	PK_GROUP_ENUM_GRAPHICS,
	PK_GROUP_ENUM_INTERNET,
	PK_GROUP_ENUM_OFFICE,
	PK_GROUP_ENUM_OTHER,
	PK_GROUP_ENUM_PROGRAMMING,
	PK_GROUP_ENUM_MULTIMEDIA,
	PK_GROUP_ENUM_SYSTEM,
	PK_GROUP_ENUM_DESKTOP_GNOME,
	PK_GROUP_ENUM_DESKTOP_KDE,
	PK_GROUP_ENUM_DESKTOP_XFCE,
	PK_GROUP_ENUM_DESKTOP_OTHER,
	PK_GROUP_ENUM_PUBLISHING,
	PK_GROUP_ENUM_SERVERS,
	PK_GROUP_ENUM_FONTS,
	PK_GROUP_ENUM_ADMIN_TOOLS,
	PK_GROUP_ENUM_LEGACY,
	PK_GROUP_ENUM_LOCALIZATION,
	PK_GROUP_ENUM_VIRTUALIZATION,
	PK_GROUP_ENUM_SECURITY,
	PK_GROUP_ENUM_POWER_MANAGEMENT,
	PK_GROUP_ENUM_COMMUNICATION,
	PK_GROUP_ENUM_NETWORK,
	PK_GROUP_ENUM_MAPS,
	PK_GROUP_ENUM_REPOS,
	PK_GROUP_ENUM_UNKNOWN,
  /* Other values */
  YPKG_GROUP_SUGGESTED,
  YPKG_GROUP_RECOMMENDED,
  YPKG_GROUP_ALL,
} YPkgGroupEnum;


YPkgGroupEnum zypp_tag_convert (const std::string &rpm_group);

const char *zypp_tag_group_enum_to_localised_text (YPkgGroupEnum group);
const char *zypp_tag_enum_to_icon (YPkgGroupEnum group);

