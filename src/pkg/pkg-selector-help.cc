/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/*
  Textdomain "gtk"
 */

#include "YGi18n.h"

const char *pkg_help[] = {
	_("<h1>Purpose</h1>"
	"<p>This tool lets you install, remove, and update applications.</p>"
	"<p>Software in &product; is broken down and distributed in the form of "
	"packages. This way, if multiple applications require a common system file, "
	"this system file is shipped in its own package and is installed only once "
	"if needed. The user need not be concerned about such underlying <i>dependencies</i>. "
	"Likewise, the plugins and other non-essential data of a given application may "
	"be shipped in their own packages, so the user may install them only if needed.</p>"),
	_("<p>Common suffixes for complementary packages:</p>"
	"<ul>"
	"<li><b>-plugin-</b>: extends the application with extra functionality.</li>"
	"<li><b>-devel</b>: headers for software development.</li>"
	"<li><b>-debuginfo</b>: debug symbols for software testing.</li>"
	"<li><b>-fr</b>, <b>-pl</b> or other language siglas: translation files (your language "
	"will be marked for installation automatically when needed).</li>"
	"</ul>"),
	_("<p>Both the packages that are installed on your system, and the packages "
	"that are available from the <i>repositories</i> you have configured will be listed "
	"together. <i>Status</i> filters are available in the right-bottom box.</p>"),
	_("<blockquote>A repository is a packages media; it can either be local (such as the "
	"installation CD), or a remote internet server. You can find an utility to setup "
	"repositories on the YaST control center, which can also be accessed via the "
	"<b>Configuration > Repositories</b> menu item.</blockquote>"),
	_("<h1>Usage</h1>"),
	_("<h2>Install, Upgrade, Remove, Undo tab pages</h2>"
	"<p>All packages are listed together unless you have selected a <i>status</i> filter "
	"from the right-bottom box. The check-box next to the package name indicates whether "
	"the package is installed or not. If a more recent version of an installed "
	"package is available, the version text will be highlighted in blue and an "
	"upgrade button conveniently placed next to it. It is highlighted red if the "
	"version installed is no longer being made available in any configured repository.</p>"
	"<p>The context menu (right click on a package) provides extra options. "
	"The <b>Undo</b> option can be used to revert any changes you have made. Multiple "
	"packages may be selected (using the Control key) and modified together.</p>"
	"<p>Use the <b>Version</b> list over the description box to select a specific version "
	"of a package.</p>"
	"<p>Press the <b>Apply</b> button when you want your changes to be performed.</p>"),
	_("<h2>Lock software</h2>"
	"<p>Packages can be locked against automatic changes via the context menu.</p>"
	"<p>Locking is only useful in very unusual cases: for instance, you may not want "
	"to install a given driver because it interferes with your system, yet you want "
	"to install some collection that includes it. Locks can be applied whether the "
	"package is installed or not.</p>"),
	_("<h2>Filters</h2>"),
	_("<h3>Search</h3>"
	"<p>Enter free text into the search-field to match their names and descriptions. "
	"(a search for 'office' will bring up the 'LibreOffice' packages as well as "
	"'AbiWord' which carries the word 'office' in its description). You can search for "
	"multiple keywords by separating the with a white space (e.g. 'spread sheet' "
	"would return 'libreoffice-calc').You may use the "
	"search combined with a filter, like searching for a package in a given repository. "
	"Other search attributes are provided, such as to search for a given file.</p>"),
	_("<h3>Groups</h3>"
	"<p>Software for &product; is indexed so that you can find software for a specific "
	"task when you are not aware of the software selection available. A more detailed, "
	"hierarchical classification is provided by the <b>RPM Groups</b> filter.</p>"),
	_("<h3>Patterns and Languages</h3>"
	"<p><b>Patterns</b> are task-oriented collections of multiple packages that "
	"install like one. The installation of the <i>File Server</i> pattern, for example, "
	"will install various packages needed for running such a server.</p>"
	"<p>If you want to install a particular language, you may want to do so via the "
	"<b>Language</b> tool from the YaST control center.</p>"),
	_("<h2>Software details in the box below</h2>"
	"<p>Explore the available information about the package in the box below. Note "
	"that more information is available for installed packages than for those only "
	"available from a repository.</p>"
	"<p>You can also pick a specific version of the package to install from this "
	"box.</p>"),
	"<hr/><p>http://en.opensuse.org/YaST2-GTK</p>",
	0
};

const char *patch_help[] = {
	_("<h1>Purpose</h1>"
	"<p>This tool gives you control on overviewing and picking patches. You may also "
	"reverse patches that have been applied to the system.</p>"),
	_("<h1>Usage</h1>"
	"<h2>Categories</h2>"
	"<p>Patches are grouped as follows:</p>"
	"<ul>"
	"<li><b>Security</b>: patches a software flaw that could be exploited to gain "
	"restricted privilege.</li>"
	"<li><b>Recommended</b>: fixes non-security related flaws (e.g. data corruption, "
	"performance slowdown)</li>"
	"<li><b>Optional</b>: ones that only apply to few users.</li>"
	"<li><b>Documentation</b>: fixes documentation errors.</li>"
	"<li><b>YaST</b>: patches for the YaST control center tools.</li>"
	"</ul>"),
	_("<p>Only patches that apply to your system will be visible. You can be sure "
	"that the decision to make a patch available is not done trivially.</p>"
	"<p>If you are looking for applications enhancements, you should check for <i>upgrades</i> "
	"on the <b>Software Manager</b>.</p>"),
	0
};

