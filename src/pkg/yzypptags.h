/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Converts RPM to PackageKit-like terminology.
   Code from yast-qt-pkg.
*/

typedef enum {
  /* PackageKit values  */
	PK_GROUP_ENUM_OFFICE,
	PK_GROUP_ENUM_PUBLISHING,
	PK_GROUP_ENUM_GRAPHICS,
	PK_GROUP_ENUM_MULTIMEDIA,
	PK_GROUP_ENUM_EDUCATION,
	PK_GROUP_ENUM_GAMES,
	PK_GROUP_ENUM_DESKTOP_GNOME,
	PK_GROUP_ENUM_DESKTOP_KDE,
	PK_GROUP_ENUM_DESKTOP_XFCE,
	PK_GROUP_ENUM_DESKTOP_OTHER,
	PK_GROUP_ENUM_COMMUNICATION,
	PK_GROUP_ENUM_NETWORK,
	PK_GROUP_ENUM_PROGRAMMING,
	PK_GROUP_ENUM_DOCUMENTATION,
	PK_GROUP_ENUM_ADMIN_TOOLS,
	PK_GROUP_ENUM_SECURITY,
	PK_GROUP_ENUM_LOCALIZATION,
	PK_GROUP_ENUM_SYSTEM,
	PK_GROUP_ENUM_UTILITIES,
	PK_GROUP_ENUM_UNKNOWN,
	PK_GROUP_ENUM_SIZE,
} YPkgGroupEnum;


YPkgGroupEnum zypp_tag_convert (const std::string &rpm_group);

const char *zypp_tag_group_enum_to_localised_text (YPkgGroupEnum group);
const char *zypp_tag_enum_to_icon (YPkgGroupEnum group);

