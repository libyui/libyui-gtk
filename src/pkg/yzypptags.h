/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Converts RPM to PackageKit-like terminology.
   Code from yast-qt-pkg.
*/

typedef enum {
  /* PackageKit values  */
	YPKG_GROUP_OFFICE,
	YPKG_GROUP_PUBLISHING,
	YPKG_GROUP_GRAPHICS,
	YPKG_GROUP_MULTIMEDIA,
	YPKG_GROUP_EDUCATION,
	YPKG_GROUP_GAMES,
	YPKG_GROUP_DESKTOP_GNOME,
	YPKG_GROUP_DESKTOP_KDE,
	YPKG_GROUP_DESKTOP_XFCE,
	YPKG_GROUP_DESKTOP_OTHER,
	YPKG_GROUP_COMMUNICATION,
	YPKG_GROUP_NETWORK,
	YPKG_GROUP_PROGRAMMING,
	YPKG_GROUP_DOCUMENTATION,
	YPKG_GROUP_ADMIN_TOOLS,
	YPKG_GROUP_SECURITY,
	YPKG_GROUP_LOCALIZATION,
	YPKG_GROUP_SYSTEM,
	YPKG_GROUP_UTILITIES,
	YPKG_GROUP_UNKNOWN,
	YPKG_GROUP_SUGGESTED,
	YPKG_GROUP_RECOMMENDED,
	YPKG_GROUP_ORPHANED,
	YPKG_GROUP_RECENT,
	YPKG_GROUP_MULTIVERSION,
	YPKG_GROUP_TOTAL,
} YPkgGroupEnum;


YPkgGroupEnum zypp_tag_convert (const std::string &rpm_group);

const char *zypp_tag_group_enum_to_localised_text (YPkgGroupEnum group);
const char *zypp_tag_enum_to_icon (YPkgGroupEnum group);

