/*
  Textdomain "yast2-gtk"
 */

static const char *pkg_help[] = {
	_("<h1>Purpose</h1>"
	"<p>This tool lets you install, remove, and update applications.</p>"
	"<p>&product; 's software management is also called 'package management'. A package is "
	"generally an application bundle, but multiple packages that extend the application "
	"may be offered in order to avoid clutter (e.g. games tend to de-couple the music "
	"data in another package, since its not essential and requires significant disk space). "
	"The base package will get the application's name, while extra packages are suffix-ed. "
	"Common extras are:</p>"
	"<ul>"
	"<li>-plugin-: extends the application with some extra functionality.</li>"
	"<li>-devel: needed for software development.</li>"
	"<li>-debuginfo: needed for software beta-testing.</li>"
	"<li>-fr, -dr, -pl (language siglas): translation files (your language package will "
	"be marked for installation automatically).</li>"
	"</ul>"
	"<p>You will find both packages installed on your system, and packages that are made "
	"available through the configured repositories.</p>"
	"<blockquote>A repository is a packages media; it can either be local (such as the "
	"&product; CDs), or a remote internet server. You can find utilities to setup "
	"repositories on the YaST control center.</blockquote>"),
	_("<h1>Usage</h1>"
	"<h2>Install, Upgrade, Remove, Undo tab pages</h2>"
	"<p>These tabs produce listings of the different sources of packages. 'Install' "
	"lists available packages from the configured repositories. "
	"'Upgrade' are the subset of installed packages that have more recent versions "
	"available. 'Remove' allows you to uninstall a package that is installed in your "
	"system. You can review changes in the 'Undo' tab, and revoke changes individually "
	"at any time.</p>"
	"<p>Press the 'Apply' button when you want your changes to be performed.</p>"),
	_("<h2>Search</h2>"
	"<p>Enter free text into the search-field to match their names and descriptions. "
	"(a search for 'office' will bring up the 'OpenOffice' packages as well as "
	"AbiWord which carries the word 'office' in its description). You can also "
	"choose to view software from a specific repository. Searching other attributes "
	"is possible. For instance, you may perform a search on the installed package "
	"containing a given file. Press the search icon for the filter menu.</p>"),
	_("<h2>Groups, Patterns, Languages</h2>"
	"<p>Software for &product; is indexed so that you can find software for a specific "
	"task when you don't know the name of the software you are looking for. Browse "
	"indexes of software through the Groups pane. A more detailed, hierarchical classification "
	"of software packages, like 'Multimedia/Video' is available through the 'Detailed' check "
	"button. 'Patterns' and 'Languages' are task-oriented collections of multiple packages that "
	"install like one (the installation of the 'server'-pattern for example will install various "
	"software needed for running a server). By using 'Install All' you make sure that future "
	"collection changes, when you upgrade &product;, will be honored.</p>"),
	_("<h2>Software details in the box below</h2>"
	"<p>Explore the available information about the package in the box below. Note "
	"that more information is available for installed software than that only "
	"available from a repository.</p>"
	"<p>You can also pick a specific version of the package to install from this "
	"box.</p>"
	"<p>The box will be placed to the right on large wide-screens when on fullscreen, "
	"in order to better use the screen space.</p>"),
	_("<h2>Lock software</h2>"
	"<p>Advanced users may want to know that locking or unlocking a package against "
	"automatic changes can be perfomed by right-clicking the package list entry.</p>"
	"<p>Locking is only useful in very unusual cases: for instance, you may not want "
	"to install a given drivers because it interferes with your system, yet you want "
	"to install some collection that includes it.</p>"),
	_("<h2>Apply button</h2>"
	"<p>Changes will be performed once you decide to click the 'Apply' "
	"button in the lower-right corner. If you want to leave the software manager "
	"without performing any changes, simply press 'Cancel'.</p>"),
	_("<hr/><p>Developed for &product; by:</p>"
	"<blockquote>Ricardo Cruz &lt;rpmcruz@alunos.dcc.fc.up.pt&gt;</blockquote>" 
	"<p>Co-designed by Christian Jaeger.</p>"),
	0
};

static const char *patch_help[] = {
	_("<h1>Purpose</h1>"
	"<p>This tool gives you control on overviewing and picking patches. You may also "
	"reverse patches that have been applied to the system.</p>"
	""
	"<h1>Usage</h1>"
	"<h2>Categories</h2>"
	"<p>Patches are grouped as follows:</p>"
	"<ul>"
	"<li>Security: patches a software flaw that could be exploited to gain "
	"restricted privilege.</li>"
	"<li>Recommended: fixes non-security related flaws (e.g. data corruption, "
	"performance slowdown)</li>"
	"<li>Optional: ones that only apply to few users.</li>"
	"</ul>"
	"<p>Only patches that apply to your system will be visible. &product; developers "
	"are very restrained in pushing patches; you can be sure that all patches are "
	"of signficant severity.</p>"
	"<p>If you are looking for applications enhancements, you should check for upgrades "
	"on the Software Manager.</p>"),
	0
};

