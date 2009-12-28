/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/*
  Textdomain "yast2-gtk"
 */

/* Ypp, zypp wrapper */
// check the header file for information about this wrapper

#include "YGi18n.h"
#include "yzyppwrapper.h"
#include "yzypptags.h"
#include <string.h>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#define YUILogComponent "gtk-pkg"
#include <YUILog.h>
#include "config.h"

#include <zypp/ZYppFactory.h>
#include <zypp/ResObject.h>
#include <zypp/ResPoolProxy.h>
#include <zypp/ui/Selectable.h>
#include <zypp/Patch.h>
#include <zypp/Package.h>
#include <zypp/Pattern.h>
#include <zypp/Product.h>
#include <zypp/Repository.h>
#include <zypp/RepoManager.h>
#include <zypp/sat/LocaleSupport.h>

#include <glib/gslist.h>
#include "YGUtils.h"

//** Zypp shortcuts

typedef zypp::ResPoolProxy ZyppPool;
inline ZyppPool zyppPool() { return zypp::getZYpp()->poolProxy(); }
typedef zypp::ui::Selectable::Ptr ZyppSelectable;
typedef zypp::ui::Selectable*     ZyppSelectablePtr;
typedef zypp::ResObject::constPtr ZyppObject;
typedef zypp::ResObject*          ZyppObjectPtr;
typedef zypp::Package::constPtr   ZyppPackage;
typedef zypp::Patch::constPtr     ZyppPatch;
typedef zypp::Pattern::constPtr   ZyppPattern;
inline ZyppPackage tryCastToZyppPkg (ZyppObject obj)
{ return zypp::dynamic_pointer_cast <const zypp::Package> (obj); }
inline ZyppPatch tryCastToZyppPatch (ZyppObject obj)
{ return zypp::dynamic_pointer_cast <const zypp::Patch> (obj); }
inline ZyppPattern tryCastToZyppPattern (ZyppObject obj)
{ return zypp::dynamic_pointer_cast <const zypp::Pattern> (obj); }

//** Utilities

// converts a set of tree representation in a form of a strings to a tree structure.
// String tree representations are, for instance, filenames: /dir1/dir2/file
struct StringTree {
	typedef int (*Compare)(const char *, const char *);
	Compare compare;
	char delim;
	const char *trans_domain;
	GNode *root;

	StringTree (Compare compare, char delim, const char *trans_domain)
	: compare (compare), delim (delim), trans_domain (trans_domain)
	{
		// the root is a dummy node to keep GNode happy
		root = g_node_new (NULL);
	}

	~StringTree()
	{
		struct inner {
			static void free (GNode *node, void *_data)
			{ delete ((Ypp::Node *) node->data); }
		};
		g_node_children_foreach (root, G_TRAVERSE_ALL, inner::free, NULL);
		g_node_destroy (root);
	}

	Ypp::Node *getFirst()
	{
		if (root->children)
			return (Ypp::Node *) root->children->data;
		return NULL;
	}

	Ypp::Node *add (const std::string &tree_str, const std::string &order)
	{  // NOTE: returns NULL for empty strings
		const gchar delimiter[2] = { delim, '\0' };
		gchar **nodes_str = g_strsplit (tree_str.c_str(), delimiter, -1);

		GNode *parent = root, *sibling = 0;
		Ypp::Node *ret = 0;
		gchar **i;
		for (i = nodes_str; *i; i++) {
			if (!**i)
				continue;
			const char *str = *i;
			if (trans_domain)
				str = dgettext (trans_domain, str);
			bool found = false;
			if (!order.empty())
				// when ordered, make sure it already doesn't exist with another order
				for (sibling = parent->children; sibling; sibling = sibling->next) {
					Ypp::Node *node = (Ypp::Node *) sibling->data;
					int cmp = (*compare) (str, node->name.c_str());
					if (cmp == 0) {
						found = true;
						ret = node;
						break;
					}
				}
			if (!found) {
				const char *s1 = order.empty() ? str : order.c_str();
				for (sibling = parent->children; sibling; sibling = sibling->next) {
					Ypp::Node *node = (Ypp::Node *) sibling->data;
					const char *s2 = order.empty() ? node->name.c_str() : node->order.c_str();
					int cmp = (*compare) (s1, s2);
					if (cmp == 0) {
						found = true;
						ret = node;
						break;
					}
					else if (cmp < 0)
						break;
				}
			}
			if (!found)
				break;
			parent = sibling;
		}

		for (; *i; i++) {
			Ypp::Node *node = new Ypp::Node();
			GNode *n = g_node_new ((void *) node);
			const char *str = *i;
			if (trans_domain)
				str = dgettext (trans_domain, str);
			node->name = str;
			node->order = order;
			node->icon = NULL;
			node->impl = (void *) n;
			g_node_insert_before (parent, sibling, n);
			parent = n;
			sibling = NULL;
			ret = node;
		}
		g_strfreev (nodes_str);
		return ret;
	}
};

//** Singleton

static Ypp *ypp = 0;

Ypp *Ypp::get()
{
	if (!ypp)
		ypp = new Ypp();
	return ypp;
}

void Ypp::finish()
{
	delete ypp; ypp = NULL;
}

// Ypp::Impl declaration, to expose some methods for usage
struct Ypp::Impl
{
public:
	Impl();
	~Impl();

	const Repository *getRepository (int nb);
	const Repository *getRepository (const std::string &zyppId);
	zypp::RepoInfo getRepoInfo (const Repository *repo);
	Disk *getDisk();

	// for Packages
	bool acceptLicense (Ypp::Package *package, const std::string &license);
	void notifyMessage (Ypp::Package *package, const std::string &message);
	void packageModified (Ypp::Package *package);

	// for the Primitive Pools
	PkgList *getPackages (Package::Type type);

	Ypp::Node *mapCategory2Enum (YPkgGroupEnum group);

private:
	bool resolveProblems();
	Node *addCategory (Ypp::Package::Type type, const std::string &str, const std::string &order);
	void polishCategories (Ypp::Package::Type type);

	void startTransactions();
	void finishTransactions();

	friend class Ypp;
	PkgList *packages [Package::TOTAL_TYPES];  // primitive pools
	StringTree *categories [Package::TOTAL_TYPES], *categories2;
	Ypp::Node *mapCategories2 [PK_GROUP_ENUM_SIZE];
	std::vector <Repository *> repos;
	const Repository *favoriteRepo;
	int favoriteRepoPriority;
	Disk *disk;
	Interface *interface;
	GSList *pkg_listeners;
	bool inTransaction;
	GSList *transactions;
};

//** Package

struct Ypp::Package::Impl
{
	/* Ypp::Package serves as a proxy to this class which is derived into
	   PackageSel for packages, patterns and patches, and PackageLang for locales. */

Type type;
Package *m_parent;
std::string m_name, m_summary;	

	Impl (Type type) : type (type) {}

	virtual std::string name() = 0;
	virtual std::string summary() = 0;
	virtual Node *category() { return NULL; }
	virtual Node *category2() { return NULL; }
	virtual bool containsPackage (const Ypp::Package *package) = 0;
	virtual void containsStats (int *installed, int *total) = 0;

	virtual std::string description (MarkupType markup) = 0;
	virtual std::string filelist (MarkupType markup) { return ""; }
	virtual std::string changelog()           { return ""; }
	virtual std::string authors (MarkupType markup)  { return ""; }
	virtual std::string support()             { return ""; }
	virtual std::string supportText (MarkupType markup) { return ""; }
	virtual std::string size()                { return ""; }
	virtual std::string icon() = 0;
	virtual bool isRecommended() const       { return false; }
	virtual bool isSuggested() const         { return false; }
	virtual int  buildAge() const            { return 0; }
	virtual bool isSupported() const       { return true; }
	virtual int severity() const             { return 0; }

	virtual std::string provides (MarkupType markup) const { return ""; }
	virtual std::string requires (MarkupType markup) const { return ""; }

	virtual const Ypp::Package::Version *getInstalledVersion() { return false; }
	virtual const Ypp::Package::Version *getAvailableVersion (int nb) { return false; }

	virtual bool isInstalled() = 0;
	virtual bool hasUpgrade() = 0;
	virtual bool isLocked() = 0;

	virtual bool toInstall (const Ypp::Package::Version **repo = 0) = 0;
	virtual bool toRemove() = 0;
	virtual bool toModify() = 0;
	virtual bool isAuto() = 0;

	virtual void install (const Ypp::Package::Version *repo) = 0;
	virtual void remove() = 0;
	virtual void undo() = 0;
	virtual bool canLock() = 0;
	virtual bool canRemove() = 0;
	virtual void lock (bool lock) = 0;

	// internal: did the resolver touch it
	virtual bool isTouched() = 0;
	virtual void setNotTouched() = 0;
};

Ypp::Package::Package (Impl *impl) : impl (impl) { impl->m_parent = this; }
Ypp::Package::~Package() { delete impl; }

Ypp::Package::Type Ypp::Package::type() const { return impl->type; }

const std::string &Ypp::Package::name() const
{
	if (impl->m_name.empty())
		impl->m_name = const_cast <Impl *> (impl)->name();
	return impl->m_name;
}

const std::string &Ypp::Package::summary()
{
	if (impl->m_summary.empty())
		impl->m_summary = const_cast <Impl *> (impl)->summary();
	return impl->m_summary;
}

Ypp::Node *Ypp::Package::category()                { return impl->category(); }
Ypp::Node *Ypp::Package::category2()               { return impl->category2(); }
bool Ypp::Package::containsPackage (const Ypp::Package *package) const
{ return const_cast <Impl *> (impl)->containsPackage (package); }
void Ypp::Package::containsStats (int *installed, int *total) const
{ const_cast <Impl *> (impl)->containsStats (installed, total); }

std::string Ypp::Package::description (MarkupType markup) { return impl->description (markup); }
std::string Ypp::Package::filelist (MarkupType markup)    { return impl->filelist (markup); }
std::string Ypp::Package::changelog()             { return impl->changelog(); }
std::string Ypp::Package::authors (MarkupType markup)     { return impl->authors (markup); }
std::string Ypp::Package::support()               { return impl->support(); }
std::string Ypp::Package::supportText (MarkupType markup) { return impl->supportText (markup); }
std::string Ypp::Package::size() { return impl->size(); }
std::string Ypp::Package::icon()                  { return impl->icon(); }
bool Ypp::Package::isRecommended() const          { return impl->isRecommended(); }
bool Ypp::Package::isSuggested() const            { return impl->isSuggested(); }
int Ypp::Package::buildAge() const                { return impl->buildAge(); }
bool Ypp::Package::isSupported() const          { return impl->isSupported(); }

int Ypp::Package::severity() const                { return impl->severity(); }

std::string Ypp::Package::severityStr (int id)
{
	switch (id) {
		case 0: return _("Security");
		case 1: return _("Recommended");
		case 2: return "YaST";
		case 3: return _("Documentation");
		case 4: return _("Optional");
		case 5: default: break;
	}
	return _("Other");
}


std::string Ypp::Package::provides (MarkupType markup) const { return impl->provides (markup); }
std::string Ypp::Package::requires (MarkupType markup) const { return impl->requires (markup); }

std::string Ypp::Package::getPropertyStr (const std::string &prop, MarkupType markup)
{
	if (prop == "name")
		return name();
	if (prop == "summary")
		return summary();
	if (prop == "description")
		return description (markup);
	if (prop == "filelist")
		return filelist (markup);
	if (prop == "support")
		return support();
	if (prop == "size")
		return size();
	if (prop == "available-version") {
		const Ypp::Package::Version *version = getAvailableVersion (0);
		if (version)
			return version->number;
		return "";
	}
	if (prop == "repository") {
		const Ypp::Package::Version *version = 0;
		if (!toInstall (&version))
			version = getInstalledVersion();
		std::string repo;
		if (version && version->repo)
			return version->repo->name;
		return "";
	}
	yuiError() << "No string property: " << prop << std::endl;
	return "";
}

int Ypp::Package::getPropertyInt (const std::string &prop)
{
	if (prop == "severity")
		return severity();
	yuiError() << "No integer property: " << prop << std::endl;
	return 0;
}

bool Ypp::Package::getPropertyBool (const std::string &prop)
{
	if (prop == "is-recommended")
		return isRecommended();
	if (prop == "is-suggested")
		return isSuggested();
	if (prop == "is-supported")
		return isSupported();
	if (prop == "to-install")
		return toInstall();
	yuiError() << "No boolean property: " << prop << std::endl;
	return false;
}

const Ypp::Package::Version *Ypp::Package::getInstalledVersion()
{ return impl->getInstalledVersion(); }
const Ypp::Package::Version *Ypp::Package::getAvailableVersion (int nb)
{ return impl->getAvailableVersion (nb); }
const Ypp::Package::Version *Ypp::Package::fromRepository (const Repository *repo)
{
	for (int i = 0; getAvailableVersion (i); i++) {
		const Version *version = getAvailableVersion (i);
		if (version->repo == repo)
			return version;
	}
	return NULL;
}

bool Ypp::Package::isInstalled() { return impl->isInstalled(); }
bool Ypp::Package::hasUpgrade()  { return impl->hasUpgrade(); }
bool Ypp::Package::isLocked()    { return impl->isLocked(); }

bool Ypp::Package::toInstall (const Ypp::Package::Version **version)
{ return impl->toInstall (version); }
bool Ypp::Package::toRemove()   { return impl->toRemove(); }
bool Ypp::Package::toModify()   { return impl->toModify(); }
bool Ypp::Package::isAuto()     { return impl->isAuto(); }

void Ypp::Package::install (const Version *version)
{
	impl->install (version);
	ypp->impl->packageModified (this);
}

void Ypp::Package::remove()
{
	impl->remove();
	ypp->impl->packageModified (this);
}

void Ypp::Package::undo()
{
	impl->undo();
	ypp->impl->packageModified (this);
}

bool Ypp::Package::canLock() { return impl->canLock(); }

bool Ypp::Package::canRemove() { return impl->canRemove(); }

void Ypp::Package::lock (bool lock)
{
	impl->lock (lock);
	ypp->impl->packageModified (this);
}

struct PackageSel : public Ypp::Package::Impl
{
ZyppSelectable m_sel;
Ypp::Node *m_category, *m_category2;
GSList *m_availableVersions;
Ypp::Package::Version *m_installedVersion;
bool m_isInstalled, m_hasUpgrade;
zypp::ui::Status m_curStatus;  // so we know if resolver touched it
// for Patterns
GSList *m_containsPackages;
int m_installedPkgs, m_totalPkgs;

	PackageSel (Ypp::Package::Type type, ZyppSelectable sel, Ypp::Node *category, Ypp::Node *category2)
	: Impl (type), m_sel (sel), m_category (category), m_category2 (category2),
	  m_availableVersions (NULL), m_installedVersion (NULL), m_containsPackages (NULL)
	{
		// for patterns, there is no reliable way to know if it's installed (because
		// if it set to be installed that will alter candidate's satisfied status.)
		if (type == Ypp::Package::PATTERN_TYPE || type == Ypp::Package::PATCH_TYPE)
			m_isInstalled = sel->candidateObj().isSatisfied() &&
				// from yast-qt, may be it is satisfied because is preselected:
				!sel->candidateObj().status().isToBeInstalled();
		else {
			if (sel->installedEmpty())
				m_isInstalled = false;
			else
				m_isInstalled = !sel->installedObj().isBroken();
		}
		// don't use getAvailableVersion(0) for hasUpgrade() has its inneficient.
		// let's just cache candidate() at start, which should point to the newest version.
		const ZyppObject candidate = sel->candidateObj();
		const ZyppObject installed = sel->installedObj();
		m_hasUpgrade = false;
		if (!!candidate && !!installed)
			m_hasUpgrade = zypp::Edition::compare (candidate->edition(), installed->edition()) > 0;

		setNotTouched();
		m_installedPkgs = m_totalPkgs = 0;
	}

	virtual ~PackageSel()
	{
		delete m_installedVersion;
		for (GSList *i = m_availableVersions; i; i = i->next)
			delete ((Ypp::Package::Version *) i->data);
		g_slist_free (m_availableVersions);
		g_slist_free (m_containsPackages);
	}

	virtual bool isTouched()
	{ return m_curStatus != m_sel->status(); }
	virtual void setNotTouched()
	{ m_curStatus = m_sel->status(); }

	GSList *getContainedPackages()
	{
		if (!m_containsPackages) {
			switch (type) {
				case Ypp::Package::PATTERN_TYPE: {
					ZyppObject object = m_sel->theObj();
					ZyppPattern pattern = tryCastToZyppPattern (object);
					zypp::Pattern::Contents contents (pattern->contents());
					for (zypp::Pattern::Contents::Selectable_iterator it =
						 contents.selectableBegin(); it != contents.selectableEnd(); it++) {
						ZyppSelectablePtr sel = get_pointer (*it);
						m_containsPackages = g_slist_append (m_containsPackages, sel);
						if (!sel->installedEmpty())
							m_installedPkgs++;
						m_totalPkgs++;
					}
					break;
				}
				case Ypp::Package::PATCH_TYPE: {
					ZyppObject object = m_sel->theObj();
					ZyppPatch patch = tryCastToZyppPatch (object);
					zypp::Patch::Contents contents (patch->contents());
					for (zypp::Patch::Contents::Selectable_iterator it =
					     contents.selectableBegin(); it != contents.selectableEnd(); it++) {
						ZyppSelectablePtr sel = get_pointer (*it);
						m_containsPackages = g_slist_append (m_containsPackages, sel);
						if (!sel->installedEmpty())
							m_installedPkgs++;
						m_totalPkgs++;
					}
					break;
				}
				default:
					break;
			}
		}
		return m_containsPackages;
	}

	virtual std::string name()
	{
		if (type == Ypp::Package::PATTERN_TYPE)
			return m_sel->theObj()->summary();
		return m_sel->name();
	}

	virtual std::string summary()
	{
		if (type == Ypp::Package::PATTERN_TYPE) {
			int installed, total;
			containsStats (&installed, &total);
			std::ostringstream stream;
			stream << _("Installed: ") << installed << _(" of ") << total;
			return stream.str();
		}
		return m_sel->theObj()->summary();
	}

	virtual std::string description (MarkupType markup)
	{
		ZyppObject object = m_sel->theObj();
		std::string text = object->description(), br = "<br>";
		if (markup == GTK_MARKUP && type == Ypp::Package::PACKAGE_TYPE) {
			text = YGUtils::escapeMarkup (text);
			text += "\n";
			const Ypp::Package::Version *version;
			version = getInstalledVersion();
			if (version)
				text += std::string ("\n") + _("Installed:") + " " + version->number;
			version = getAvailableVersion (0);
			if (version) {
				text += std::string ("\n") + _("Candidate:") + " " + version->number;
				text += std::string (" (") + _("from") + " " + version->repo->name + ")";
			}
			return text;
		}
		if (markup == NO_MARKUP)
			return text;

		switch (type) {
			case Ypp::Package::PACKAGE_TYPE:
			{
				// if it has this header, then it is HTML
				const char *header = "<!-- DT:Rich -->", header_len = 16;
				if (!text.compare (0, header_len, header, header_len))
					;
				else {
					// cut authors block
					std::string::size_type i = text.find ("\nAuthors:", 0);
					if (i == std::string::npos)
						i = text.find ("\nAuthor:", 0);
					if (i != std::string::npos)
						text.erase (i);
					// cut any lines at the end
					while (text.length() > 0 && text [text.length()-1] == '\n')
						text.erase (text.length()-1);

					text = YGUtils::escapeMarkup (text);
					YGUtils::replace (text, "\n\n", 2, "<br>");  // break every double line
					text += br;
				}

				if (!isInstalled() && (isRecommended() || isSuggested())) {
					text += "<font color=\"#715300\">";
					if (isRecommended())
						text += _("(As this package is an extension to an already installed package, it is <b>recommended</b> it be installed.)");
					if (isSuggested())
						text += _("(As this package complements some installed packages, it is <b>suggested</b> it be installed.)");
					text += "</font>" + br;
				}

				// specific
				ZyppPackage package = tryCastToZyppPkg (object);
#if ZYPP_VERSION >= 5013001
				text += br + "<b>" + _("Size:") + "</b> " + object->installSize().asString();
#else
				text += br + "<b>" + _("Size:") + "</b> " + object->installsize().asString();
#endif
				std::string url = package->url(), license = package->license();
				if (!url.empty())
					text += br + "<b>" + _("Web site:") + "</b> <a href=\"" + url + "\">" + url + "</a>";
				if (!license.empty())
					text += br + "<b>" + _("License:") + "</b> " + license;
#if 0
				text += br + "<b>" + _("Category:") + "</b> " + package->group();
#endif
#if 0  // show "Installed at:" and "Last build:" info
				bool hasCandidate = m_sel->hasCandidateObj();
				if (isInstalled()) {
					text += br + "<b>" + _("Installed at:") + "</b> " + m_sel->installedObj()->installtime().form("%x");
					if (!hasUpgrade())
						hasCandidate = false;  // only for upgrades
					if (hasCandidate)
						text += "&nbsp;&nbsp;&nbsp;";
				}
				else if (hasCandidate)
					text += br;
				if (hasCandidate)
					text += std::string ("<b>") + _("Last build:") + "</b> " + m_sel->candidateObj()->buildtime().form("%x");
#endif
				break;
			}
			case Ypp::Package::PATCH_TYPE:
			{
				ZyppPatch patch = tryCastToZyppPatch (object);
				if (patch->rebootSuggested()) {
					text += br + br + "<b>" + _("Reboot required: ") + "</b>";
					text += _("the system will have to be restarted in order for "
						"this patch to take effect.");
				}
				if (patch->reloginSuggested()) {
					text += br + br + "<b>" + _("Relogin required: ") + "</b>";
					text += _("you must logout and login again for "
						"this patch to take effect.");
				}
				if (patch->referencesBegin() != patch->referencesEnd()) {
					text += br + br + "<b>Bugzilla:</b><ul>";
					for (zypp::Patch::ReferenceIterator it = patch->referencesBegin();
						 it != patch->referencesEnd(); it++)
						text += "<li><a href=\"" + it.href() + "\">" + it.title() + "</a></li>";
					text += "</ul>";
				}
				break;
			}
			case Ypp::Package::PATTERN_TYPE:
			{
				int installed, total;
				containsStats (&installed, &total);
				std::ostringstream stream;
				if (markup == HTML_MARKUP)
					stream << "<br><br>";
				stream << "\n\n" << _("Installed: ") << installed << _(" of ") << total;
				text += stream.str();
				break;
			}
			default:
				break;
		}
		return text;
	}

	virtual std::string filelist (MarkupType markup)
	{
		std::string text;
		ZyppObject object = m_sel->installedObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		if (package) {
			if (markup == HTML_MARKUP) {
				StringTree tree (strcmp, '/', NULL);

#if ZYPP_VERSION > 5024005
				zypp::Package::FileList files = package->filelist();
				for (zypp::Package::FileList::iterator it = files.begin();
					 it != files.end(); it++)
					tree.add (*it, "");
#else
				std::list <std::string> files (package->filenames());
				for (std::list <std::string>::iterator it = files.begin();
					 it != files.end(); it++)
					tree.add (*it, "");
#endif

				struct inner {
					static std::string getPath (GNode *node)
					{
						Ypp::Node *yNode  = (Ypp::Node *) node->data;
						if (!yNode)
							return std::string();
						return getPath (node->parent) + "/" + yNode->name;
					}
					static bool hasNextLeaf (GNode *node)
					{
						GNode *i;
						for (i = node->next; i; i = i->next)
							if (!i->children)
								return true;
						return false;
					}
					static bool hasPrevLeaf (GNode *node)
					{
						GNode *i;
						for (i = node->prev; i; i = i->prev)
							if (!i->children)
								return true;
						return false;
					}
					static gboolean traverse (GNode *node, void *_data)
					{
						Ypp::Node *yNode  = (Ypp::Node *) node->data;
						if (yNode) {
							std::string *str = (std::string *) _data;
							if (!hasPrevLeaf (node)) {
								std::string path = getPath (node->parent);
								*str += "<a href=" + path + ">" + path + "</a>";
								*str += "<blockquote>";
							}
							else
								*str += ", ";
							*str += yNode->name;

							if (!hasNextLeaf (node))
								*str += "</blockquote>";
						}
						return FALSE;
					}
				};
				g_node_traverse (tree.root, G_LEVEL_ORDER, G_TRAVERSE_LEAFS, -1,
						         inner::traverse, (void *) &text);
			}
			else {
#if ZYPP_VERSION > 5024005
				zypp::Package::FileList files = package->filelist();
				for (zypp::Package::FileList::iterator it = files.begin();
					 it != files.end(); it++)
					text += *it + " ";
#else
				std::list <std::string> files (package->filenames());
				for (std::list <std::string>::iterator it = files.begin();
					 it != files.end(); it++)
					text += *it + " ";
#endif
			}
		}
		return text;
	}

	virtual std::string changelog()
	{
		std::string text;
		ZyppObject object = m_sel->installedObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		if (package) {
			const std::list <zypp::ChangelogEntry> &changelogList = package->changelog();
			for (std::list <zypp::ChangelogEntry>::const_iterator it = changelogList.begin();
				 it != changelogList.end(); it++) {
				std::string date (it->date().form ("%d %B %Y")), author (it->author()),
					        changes (it->text());
				author = YGUtils::escapeMarkup (author);
				changes = YGUtils::escapeMarkup (changes);
				YGUtils::replace (changes, "\n", 1, "<br>");
				if (author.compare (0, 2, "- ", 2) == 0)  // zypp returns a lot of author strings as
					author.erase (0, 2);                  // "- author". wtf?
				text += date + " (" + author + "):<br><blockquote>" + changes + "</blockquote>";
			}
		}
		return text;
	}

	virtual std::string authors (MarkupType markup)
	{
		std::string text;
		ZyppObject object = m_sel->theObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		if (package) {
			std::string packager = package->packager(), vendor = package->vendor(), authors;
			packager = YGUtils::escapeMarkup (packager);
			vendor = YGUtils::escapeMarkup (vendor);
			const std::list <std::string> &authorsList = package->authors();
			for (std::list <std::string>::const_iterator it = authorsList.begin();
				 it != authorsList.end(); it++) {
				std::string author (*it);
				if (markup != NO_MARKUP)
					author = YGUtils::escapeMarkup (author);
				if (!authors.empty()) {
					if (markup == HTML_MARKUP)
						authors += "<br>";
					else
						authors += "\n";
				}
				authors += author;
			}
			// look for Authors line in description
			std::string description = package->description();
			std::string::size_type i = description.find ("\nAuthors:\n-----", 0);
			if (i != std::string::npos) {
				i += sizeof ("\nAuthors:\n----");
				i = description.find ("\n", i);
				if (i != std::string::npos)
					i++;
			}
			else {
				i = description.find ("\nAuthor:", 0);
				if (i == std::string::npos) {
					i = description.find ("\nAuthors:", 0);
					if (i != std::string::npos)
						i++;
				}
				if (i != std::string::npos)
					i += sizeof ("\nAuthor:");
			}
			if (i != std::string::npos) {
				std::string str = description.substr (i);
				if (markup != NO_MARKUP)
					str = YGUtils::escapeMarkup (str);
				if (markup == HTML_MARKUP)
					YGUtils::replace (str, "\n", 1, "<br>");
				authors += str;
			}
			if (markup == HTML_MARKUP) {
				if (!authors.empty()) {
					text = _("Developed by:") + ("<blockquote>" + authors) + "</blockquote>";
					if (!packager.empty() || !vendor.empty()) {
						text += _("Packaged by:");
						text += "<blockquote>";
						if (!packager.empty()) text += packager + " ";
						if (!vendor.empty()) text += "(" + vendor + ")";
						text += "</blockquote>";
					}
				}
			}
			else
				return authors;
		}
		return text;
	}

	virtual std::string support()
	{
		ZyppObject object = m_sel->theObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		if (package) {
			zypp::VendorSupportOption opt = package->vendorSupport();
			return zypp::asUserString (opt);
		}
		return "";
	}

	virtual std::string supportText (MarkupType markup)
	{
		ZyppObject object = m_sel->theObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		if (package) {
			zypp::VendorSupportOption opt = package->vendorSupport();
			std::string str (zypp::asUserStringDescription (opt));
			if (markup != HTML_MARKUP)
				return YGUtils::escapeMarkup (str);
			return str;
		}
		return "";
	}

	virtual std::string size()
	{ return m_sel->theObj()->installSize().asString(); }

	virtual std::string icon()
	{
		if (type == Ypp::Package::PATTERN_TYPE) {
			ZyppObject object = m_sel->theObj();
			ZyppPattern pattern = tryCastToZyppPattern (object);
			std::string icon = pattern->icon().asString().c_str();
			// from yast2-qt: HACK most patterns have wrong default icon
			if ((icon == zypp::Pathname("yast-system").asString()) || icon.empty())
				icon = "pattern-generic";
			// mine: remove start "./"
			else if (icon.compare (0, 2, "./", 2) == 0)
				icon.erase (0, 2);
			return icon;
		}
		return "";
	}

	virtual bool isRecommended() const
	{
		// like yast2-qt: different instances may be assigned different groups
		if (m_sel->hasCandidateObj())
			for (int i = 0; i < 2; i++) {
				ZyppObject obj;
				if (i == 0)
					obj = m_sel->candidateObj();
				else
					obj = m_sel->installedObj();
				if (obj && zypp::PoolItem (obj).status().isRecommended())
					return true;
			}
		return false;
	}

	virtual bool isSuggested() const
	{
		if (m_sel->hasCandidateObj())
			for (int i = 0; i < 2; i++) {
				ZyppObject obj;
				if (i == 0)
					obj = m_sel->candidateObj();
				else
					obj = m_sel->installedObj();
				if (obj && zypp::PoolItem (obj).status().isSuggested())
					return true;
			}
		return false;
	}

	virtual int buildAge() const
	{
		if (m_sel->hasCandidateObj()) {
			time_t build = static_cast <time_t> (m_sel->candidateObj()->buildtime());
			time_t now = time (NULL);
			return (now - build) / (60*60*24);  // in days
		}
		return -1;
	}

	virtual bool isSupported() const
	{
#if ZYPP_VERSION >= 5013001
		if (type != Ypp::Package::PACKAGE_TYPE)
			return false;
		ZyppObject object = m_sel->theObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		return !package->maybeUnsupported();
#else
		return true;
#endif
	}

	virtual int severity() const
	{
		ZyppObject object = m_sel->theObj();
		ZyppPatch patch = tryCastToZyppPatch (object);
		if (patch->category() == "security") return 0;
		if (patch->category() == "recommended") return 1;
		if (patch->category() == "yast")     return 2;
		if (patch->category() == "document") return 3;
		if (patch->category() == "optional") return 4;
		//if (patch->category() == "other")
			return 5;
	}

	virtual std::string provides (MarkupType markup) const
	{
		std::string text;
		ZyppObject object = m_sel->theObj();
		const zypp::Capabilities &capSet = object->dep (zypp::Dep::PROVIDES);
		for (zypp::Capabilities::const_iterator it = capSet.begin();
		     it != capSet.end(); it++) {
			if (!text.empty())
				text += "\n";
			text += it->asString();
		}
		if (markup != HTML_MARKUP)
			return YGUtils::escapeMarkup (text);
		return text;
	}

	virtual std::string requires (MarkupType markup) const
	{
		std::string text;
		ZyppObject object = m_sel->theObj();
		const zypp::Capabilities &capSet = object->dep (zypp::Dep::REQUIRES);
		for (zypp::Capabilities::const_iterator it = capSet.begin();
		     it != capSet.end(); it++) {
			if (!text.empty())
				text += "\n";
			text += it->asString();
		}
		if (markup != NO_MARKUP)
			return YGUtils::escapeMarkup (text);
		return text;
	}

	virtual bool containsPackage (const Ypp::Package *package)
	{
		PackageSel *sel = (PackageSel *) package->impl;
		return g_slist_find (getContainedPackages(), get_pointer (sel->m_sel)) != NULL;
	}

	virtual void containsStats (int *installed, int *total)
	{
		getContainedPackages();  // init
		*installed = m_installedPkgs;
		*total = m_totalPkgs;
	}

	virtual Ypp::Node *category()
	{ return m_category; }

	virtual Ypp::Node *category2()
	{
		if (!m_category2) {
			YPkgGroupEnum group = PK_GROUP_ENUM_UNKNOWN;
			for (int i = 0; i < 2; i++) {
				ZyppObject obj;
				if (i == 0)
					obj = m_sel->candidateObj();
				else
					obj = m_sel->installedObj();
				ZyppPackage pkg = tryCastToZyppPkg (obj);
				if (pkg) {
					group = zypp_tag_convert (pkg->group());
					if (group != PK_GROUP_ENUM_UNKNOWN)
						break;
				}
			}
			m_category2 = ypp->impl->mapCategory2Enum (group);
		}
		return m_category2;
	}

	virtual bool isInstalled()
	{
		return m_isInstalled;
	}

	virtual bool hasUpgrade()
	{
		return m_hasUpgrade;
	}

	virtual bool isLocked()
	{
		zypp::ui::Status status = m_sel->status();
		return status == zypp::ui::S_Taboo || status == zypp::ui::S_Protected;
	}

	virtual bool toInstall (const Ypp::Package::Version **version)
	{
		if (version) {
			ZyppObject candidate = m_sel->candidateObj();
			for (int i = 0; getAvailableVersion (i); i++) {
				const Ypp::Package::Version *v = getAvailableVersion (i);
				ZyppObject obj = (ZyppObjectPtr) v->impl;
				if (obj == candidate) {
					*version = v;
					break;
				}
			}
		}
		return m_sel->toInstall();
	}

	virtual bool toRemove()
	{ return m_sel->toDelete(); }

	virtual bool toModify()
	{ return m_sel->toModify(); }

	virtual bool isAuto()
	{
		zypp::ui::Status status = m_sel->status();
		return status == zypp::ui::S_AutoInstall || status == zypp::ui::S_AutoUpdate ||
			   status == zypp::ui::S_AutoDel;
	}

	virtual void install (const Ypp::Package::Version *version)
	{
		if (isLocked())
			return;
		if (!m_sel->hasLicenceConfirmed())
		{
			ZyppObject obj = m_sel->candidateObj();
			const std::string &license = obj->licenseToConfirm();
			if (!license.empty())
				if (!ypp->impl->acceptLicense (m_parent, license))
					return;
			m_sel->setLicenceConfirmed();

			const std::string &msg = obj->insnotify();
			if (!msg.empty())
				ypp->impl->notifyMessage (m_parent, msg);
		}

		zypp::ui::Status status = m_sel->status();
		switch (status) {
			// not applicable
			case zypp::ui::S_Protected:
			case zypp::ui::S_Taboo:
			case zypp::ui::S_Install:
			case zypp::ui::S_Update:
				break;
			// undo
			case zypp::ui::S_Del:
				status = zypp::ui::S_KeepInstalled;
				break;
			// nothing to do about it
			case zypp::ui::S_AutoDel:
				break;
			// action
			case zypp::ui::S_NoInst:
			case zypp::ui::S_AutoInstall:
				status = zypp::ui::S_Install;
				break;
			case zypp::ui::S_KeepInstalled:
			case zypp::ui::S_AutoUpdate:
				status = zypp::ui::S_Update;
				break;
		}

		m_sel->setStatus (status);
		if (toInstall (NULL)) {
			if (!version) {
				version = getAvailableVersion (0);
				const Ypp::Repository *repo = ypp->favoriteRepository();
				if (repo && fromRepository (repo))
					version = fromRepository (repo);
			}
			ZyppObject candidate = (ZyppObjectPtr) version->impl;
			if (!m_sel->setCandidate (candidate)) {
				yuiWarning () << "Error: Could not set package '" << name() << "' candidate to '" << version->number << "'\n";
				return;
			}
		}
	}

	virtual void remove()
	{
		if (m_sel->hasCandidateObj()) {
			const std::string &msg = m_sel->candidateObj()->delnotify();
			if (!msg.empty())
				ypp->impl->notifyMessage (m_parent, msg);
		}

		zypp::ui::Status status = m_sel->status();
		switch (status) {
			// not applicable
			case zypp::ui::S_Protected:
			case zypp::ui::S_Taboo:
			case zypp::ui::S_NoInst:
			case zypp::ui::S_Del:
				break;
			// undo
			case zypp::ui::S_Install:
				status = zypp::ui::S_NoInst;
				break;
			// nothing to do about it
			case zypp::ui::S_AutoInstall:
			case zypp::ui::S_AutoUpdate:
				break;
			case zypp::ui::S_Update:
				status = zypp::ui::S_KeepInstalled;
				break;
			// action
			case zypp::ui::S_KeepInstalled:
			case zypp::ui::S_AutoDel:
				status = zypp::ui::S_Del;
				break;
		}

		m_sel->setStatus (status);
	}

	virtual void undo()
	{
		zypp::ui::Status status = m_sel->status();
		switch (status) {
			// not applicable
			case zypp::ui::S_Protected:
			case zypp::ui::S_Taboo:
			case zypp::ui::S_NoInst:
			case zypp::ui::S_KeepInstalled:
				break;

			// undo
			case zypp::ui::S_Install:
				status = zypp::ui::S_NoInst;
				break;
			case zypp::ui::S_Update:
			case zypp::ui::S_Del:
				status = zypp::ui::S_KeepInstalled;
				break;

			// for auto status, undo them by locking them
			case zypp::ui::S_AutoInstall:
				status = zypp::ui::S_Taboo;
				break;
			case zypp::ui::S_AutoUpdate:
			case zypp::ui::S_AutoDel:
				status = zypp::ui::S_Protected;
				break;
		}

		m_sel->setStatus (status);
	}

	virtual bool canLock() { return type != Ypp::Package::PATTERN_TYPE; }
	virtual bool canRemove() { return type == Ypp::Package::PACKAGE_TYPE; }

	virtual void lock (bool lock)
	{
		undo();

		zypp::ui::Status status;
		if (lock)
			status = isInstalled() ? zypp::ui::S_Protected : zypp::ui::S_Taboo;
		else
			status = isInstalled() ? zypp::ui::S_KeepInstalled : zypp::ui::S_NoInst;

		m_sel->setStatus (status);
	}

	static Ypp::Package::Version *constructVersion (
		ZyppObject object, ZyppObject installedObj)
	{
		Ypp::Package::Version *version = new Ypp::Package::Version();
		version->number = object->edition().asString();
		version->arch = object->arch().asString();
		version->repo = ypp->impl->getRepository (object->repoInfo().alias());
		version->cmp = installedObj ? zypp::Edition::compare (object->edition(), installedObj->edition()) : 0;
		version->impl = (void *) get_pointer (object);
		return version;
	}

	virtual const Ypp::Package::Version *getInstalledVersion()
	{
		if (!m_installedVersion) {
			ZyppObject installedObj = m_sel->installedObj();
			if (type == Ypp::Package::PATCH_TYPE) {
				if (m_sel->candidateObj() && m_sel->candidateObj().isSatisfied() &&
				    !m_sel->candidateObj().status().isToBeInstalled())
					installedObj = m_sel->candidateObj();
			}
			if (installedObj)
				m_installedVersion = constructVersion (installedObj, NULL);
		}
		return m_installedVersion;
	}

	virtual const Ypp::Package::Version *getAvailableVersion (int nb)
	{
		if (!m_availableVersions) {
			ZyppObject installedObj = m_sel->installedObj();
			if (type == Ypp::Package::PATCH_TYPE) {
				if (m_sel->candidateObj() && m_sel->candidateObj().isSatisfied())
					installedObj = m_sel->candidateObj();
			}
			const ZyppObject candidateObj = m_sel->candidateObj();
			for (zypp::ui::Selectable::available_iterator it = m_sel->availableBegin();
				 it != m_sel->availableEnd(); it++) {
				if (candidateObj && (*it)->edition() == candidateObj->edition() &&
				    (*it)->arch() == candidateObj->arch())
					continue;
				Ypp::Package::Version *version = constructVersion (*it, installedObj);
				m_availableVersions = g_slist_append (m_availableVersions, version);
			}
			if (candidateObj) {  // make sure this goes first
				Ypp::Package::Version *version = constructVersion (candidateObj, installedObj);
				m_availableVersions = g_slist_prepend (m_availableVersions, version);
			}
#if 0  // let zypp order prevail
			struct inner {
				static gint version_compare (gconstpointer pa, gconstpointer pb)
				{
					ZyppObjectPtr a = (ZyppObjectPtr) ((Ypp::Package::Version *) pa)->impl;
					ZyppObjectPtr b = (ZyppObjectPtr) ((Ypp::Package::Version *) pb)->impl;
					// swapped arguments, as we want them sorted from newer to older
					return zypp::Edition::compare (b->edition(), a->edition());
				}
			};
			m_availableVersions = g_slist_sort (m_availableVersions, inner::version_compare);
#endif
		}
		return (Ypp::Package::Version *) g_slist_nth_data (m_availableVersions, nb);
	}

	virtual const Ypp::Package::Version *fromRepository (const Ypp::Repository *repo)
	{
		for (int i = 0; getAvailableVersion (i); i++)
			if (getAvailableVersion (i)->repo == repo)
				return getAvailableVersion (i);
		return NULL;
	}
};

struct PackageLang : public Ypp::Package::Impl
{
zypp::Locale m_locale;
GSList *m_containsPackages;
bool m_installed, m_setInstalled;
int m_installedPkgs, m_totalPkgs;

	PackageLang (Ypp::Package::Type type, const zypp::Locale &zyppLang)
	: Impl (type), m_locale (zyppLang), m_containsPackages (NULL)
	{
		m_installed = m_setInstalled = isSetInstalled();
		m_installedPkgs = m_totalPkgs = 0;
	}

	~PackageLang()
	{
		g_slist_free (m_containsPackages);
	}

	virtual bool isTouched()
	{ return m_setInstalled == isSetInstalled(); }
	virtual void setNotTouched()
	{ m_setInstalled = isSetInstalled(); }

	virtual std::string name()
	{ return m_locale.name(); }

	virtual std::string summary()
	{
		int installed, total;
		containsStats (&installed, &total);
		std::ostringstream stream;
		stream << _("Installed: ") << installed << _(" of ") << total;
		return stream.str();
	}

	virtual std::string description (MarkupType markup)
	{
		std::string text ("(" + m_locale.code() + ")");
		int installed, total;
		containsStats (&installed, &total);
		std::ostringstream stream;
		if (markup == HTML_MARKUP)
			stream << "<br><br>";
		stream << "\n\n" << _("Installed: ") << installed << _(" of ") << total;
		text += stream.str();
		return text;
	}

	GSList *getContainedPackages()
	{
		if (!m_containsPackages) {
			zypp::sat::LocaleSupport myLocale (m_locale);
			for_( it, myLocale.selectableBegin(), myLocale.selectableEnd() ) {
				ZyppSelectablePtr sel = get_pointer (*it);
				m_containsPackages = g_slist_append (m_containsPackages, sel);
				if (!sel->installedEmpty())
					m_installedPkgs++;
				m_totalPkgs++;
			}
        }
        return m_containsPackages;
	}

	virtual bool containsPackage (const Ypp::Package *package)
	{
		PackageSel *sel = (PackageSel *) package->impl;
		return g_slist_find (getContainedPackages(), get_pointer (sel->m_sel)) != NULL;
	}

	virtual void containsStats (int *installed, int *total)
	{
		getContainedPackages();  // init
		*installed = m_installedPkgs;
		*total = m_totalPkgs;
	}

	virtual std::string icon()
	{
		#define ICON_PATH "/usr/share/locale/l10n/"
		static int hasPath = -1;
		if (hasPath == -1)
			hasPath = g_file_test (ICON_PATH, G_FILE_TEST_IS_DIR) ? 1 : 0;
		if (hasPath) {
			std::string code (m_locale.code());
			std::string::size_type i = code.find_last_of ('_');
			if (i != std::string::npos) {
				code.erase (0, i+1);
				// down case country name
				gchar *str = g_ascii_strdown (code.c_str(), -1);
				code = str;
				g_free (str);
			}
			std::string filename (ICON_PATH);
			filename += code + "/flag.png";
			if (g_file_test (filename.c_str(), G_FILE_TEST_IS_REGULAR))
				return filename;
		}
		return "";
	}

	bool isSetInstalled()
	{ return zypp::getZYpp()->pool().isRequestedLocale (m_locale); }

	virtual bool isInstalled()
	{ return m_installed; }
	virtual bool hasUpgrade()
	{ return false; }
	virtual bool isLocked()
	{ return false; }

	virtual bool toInstall (const Ypp::Package::Version **repo)
	{ return !m_installed && isSetInstalled(); }
	virtual bool toRemove()
	{ return m_installed && !isSetInstalled(); }
	virtual bool toModify()
	{ return m_installed != isSetInstalled(); }
	virtual bool isAuto()
	{ return false; }

	virtual void install (const Ypp::Package::Version *repo)
	{
		if (!isSetInstalled())
			zypp::getZYpp()->pool().addRequestedLocale (m_locale);
	}

	virtual void remove()
	{
		if (isSetInstalled())
			zypp::getZYpp()->pool().eraseRequestedLocale (m_locale);
	}

	virtual void undo()
	{
		if (isSetInstalled())
			remove();
		else if (m_installed)
			install (0);
	}

	virtual void lock (bool lock) {}

	virtual bool canRemove() { return true; }
	virtual bool canLock() { return false; }
};

// Packages Factory
Ypp::PkgList *Ypp::Impl::getPackages (Ypp::Package::Type type)
{
	if (!packages[type]) {
		interface->loading (0);

		PkgList *list = new PkgList();
		struct inner {
			// slower -- don't use for packages -- only for languages
			static bool utf8_order (Package *a, Package *b)
			{ return g_utf8_collate (a->name().c_str(), b->name().c_str()) < 0; }
			static bool pattern_order (Package *a, Package *b)
			{
				ZyppPattern pattern1 = tryCastToZyppPattern (((PackageSel *) a->impl)->m_sel->theObj());
				ZyppPattern pattern2 = tryCastToZyppPattern (((PackageSel *) b->impl)->m_sel->theObj());
				return strcmp (pattern1->order().c_str(), pattern2->order().c_str()) < 0;
			}
			static int pk_group_order (const char *a, const char *b)
			{
				const char *unknown = zypp_tag_group_enum_to_localised_text (PK_GROUP_ENUM_UNKNOWN);
				if (!strcmp (a, unknown)) {
					if (!strcmp (b, unknown))
						return 0;
					return 1;
				}
				if (!strcmp (b, unknown)) {
					if (!strcmp (a, unknown))
						return 0;
					return -1;
				}
				return strcmp (a, b);
			}
		};

		if (type == Package::LANGUAGE_TYPE) {
			zypp::LocaleSet locales = zypp::getZYpp()->pool().getAvailableLocales();
			list->reserve (locales.size());
			for (zypp::LocaleSet::const_iterator it = locales.begin();
			     it != locales.end(); it++) {
				Package *package = new Package (new PackageLang (type, *it));
				list->append (package);
			}
		}
		else {  // Pool Selectables
			ZyppPool::const_iterator it, end;
			int size = 0;
			switch (type) {
				case Package::PACKAGE_TYPE:
					it = zyppPool().byKindBegin <zypp::Package>();
					end = zyppPool().byKindEnd <zypp::Package>();
					size = zyppPool().size(zypp::ResKind::package);

					// for the categories
					bindtextdomain ("rpm-groups", LOCALEDIR);
					bind_textdomain_codeset ("rpm-groups", "utf8");
					// layout all categories2 already and assign it to packages on request
					categories2 = new StringTree (inner::pk_group_order, '/', NULL);
					for (int i = 0; i < PK_GROUP_ENUM_SIZE; i++) {
						YPkgGroupEnum group = (YPkgGroupEnum) i;
						const char *name = zypp_tag_group_enum_to_localised_text (group);
						const char *icon = zypp_tag_enum_to_icon (group);
						Ypp::Node *node = categories2->add (name, "");
						node->icon = icon;
						mapCategories2 [i] = node;
					}
					break;
				case Package::PATTERN_TYPE:
					it = zyppPool().byKindBegin <zypp::Pattern>();
					end = zyppPool().byKindEnd <zypp::Pattern>();
					size = zyppPool().size(zypp::ResKind::pattern);
					break;
				case Package::PATCH_TYPE:
					it = zyppPool().byKindBegin <zypp::Patch>();
					end = zyppPool().byKindEnd <zypp::Patch>();
					size = zyppPool().size(zypp::ResKind::patch);
					break;
				default:
					break;
			}
			list->reserve (size);

			for (; it != end; it++) {
				Ypp::Node *category = 0, *category2 = 0;
				ZyppSelectable sel = *it;
				ZyppObject object = sel->theObj();

				// don't show if installed broken and there is no available
				if (!sel->candidateObj()) {
					if (!sel->installedEmpty() && sel->installedObj().isBroken())
						continue;
				}

				// add category and test visibility
				switch (type) {
					case Package::PACKAGE_TYPE:
					{
						ZyppPackage zpackage = tryCastToZyppPkg (object);
						if (!zpackage)
							continue;
						category = addCategory (type, zpackage->group(), "");
						break;
					}
					case Package::PATTERN_TYPE:
					{
						ZyppPattern pattern = tryCastToZyppPattern (object);
						if (!pattern || !pattern->userVisible())
							continue;
						category = addCategory (type, pattern->category(), pattern->order());
						break;
					}
					case Package::PATCH_TYPE:
					{
						ZyppPatch patch = tryCastToZyppPatch (object);
						if (!patch)
							continue;
						if (sel->candidateObj()) {
							if (!sel->candidateObj().isRelevant())
								continue;
						}

						std::string str;
#if 0  // Zypp patch->categoryEnum() seems broken: always returns the same value (opensuse 11.1)
						switch (patch->categoryEnum()) {
							case zypp::Patch::CAT_OTHER: str = _("Other"); break;
							case zypp::Patch::CAT_YAST: str = "YaST"; break;
							case zypp::Patch::CAT_SECURITY: str = _("Security"); break;
							case zypp::Patch::CAT_RECOMMENDED: str = _("Recommended"); break;
							case zypp::Patch::CAT_OPTIONAL: str = _("Optional"); break;
							case zypp::Patch::CAT_DOCUMENT: str = _("Documentation"); break;
						}
#else
						if (patch->category() == "security")
							str = _("Security");
						else if (patch->category() == "recommended")
							str = _("Recommended");
						else if (patch->category() == "yast")
							str = "YaST";
						else if (patch->category() == "document")
							str = _("Documentation");
						else if (patch->category() == "optional")
							str = _("Optional");
						else if (patch->category() == "other")
							str = _("Other");
#endif
						category = addCategory (type, str, "");
						break;
					}
					default:
						break;
				}

				Package *package = new Package (new PackageSel (type, sel, category, category2));
				list->append (package);

				int i = list->size();
				if ((i % 500) == 0)
					interface->loading (i / (float) size);
			}
		}
		// a sort merge. Don't use g_slist_insert_sorted() -- its linear
		if (type == Ypp::Package::PATTERN_TYPE)
			list->sort (inner::pattern_order);
		else if (type == Ypp::Package::LANGUAGE_TYPE)

			list->sort (inner::utf8_order);
		else
			list->sort();
		packages[type] = list;
		polishCategories (type);
		interface->loading (1);
	}
	return packages[type];
}

Ypp::Node *Ypp::Impl::mapCategory2Enum (YPkgGroupEnum group)
{ return mapCategories2 [group]; }

//** PkgList

struct Ypp::PkgList::Impl : public Ypp::PkgList::Listener
{
PkgList *parent;
std::list <PkgList::Listener *> listeners;
std::vector <Ypp::Package *> pool;
guint inited : 2, _allInstalled : 2, _allNotInstalled : 2, _allUpgradable : 2,
      _allModified : 2, _allLocked : 2, _allUnlocked : 2, _allCanLock : 2,
      _allCanRemove : 2;
int refcount;

	Impl (const Ypp::PkgList *lparent) : Listener()
	, parent (NULL), inited (false), refcount (1)
	{
		if (lparent) {
			parent = new PkgList (*lparent);
			parent->addListener (this);
		}
	}

	~Impl()
	{
		if (parent) {
			parent->removeListener (this);
			delete parent;
		}
	}

	// implementations
	int find (const Package *package) const
	{
		for (unsigned int i = 0; i < pool.size(); i++)
			if (pool[i] == package)
				return i;
		return -1;
	}

	void remove (int i)
	{
		std::vector <Package *>::iterator it = pool.begin();
		it += i;
		pool.erase (it);
	}

	void addListener (Ypp::PkgList::Listener *listener)
	{
		listeners.push_back (listener);
	}

	void removeListener (Ypp::PkgList::Listener *listener)
	{ listeners.remove (listener); }

	void buildProps() const
	{ if (!inited) const_cast <Impl *> (this)->_buildProps(); }

	void _buildProps()
	{
		inited = true;
		if (!pool.empty()) {
			_allInstalled = _allNotInstalled = _allUpgradable = _allModified =
				_allLocked = _allUnlocked = _allCanLock = _allCanRemove = true;
			for (unsigned int  i = 0; i < pool.size(); i++) {
				Package *pkg = pool[i];
				if (!pkg->isInstalled()) {
					_allInstalled = false;
					_allUpgradable = false;
				 }
				 else {
				 	_allNotInstalled = false;
				 	if (!pkg->hasUpgrade())
				 		_allUpgradable = false;
				 }
				if (pkg->toModify())
					// if modified, can't be locked or unlocked
					_allLocked = _allUnlocked = false;
				else
					_allModified = false;
				if (pkg->isLocked())
					_allUnlocked = false;
				else
					_allLocked = false;
				if (!pkg->canLock())
					_allCanLock = false;
				if (!pkg->canRemove())
					_allCanRemove = false;
			}
		}
		else
			_allInstalled = _allNotInstalled = _allUpgradable = _allModified =
				_allLocked = _allUnlocked = _allCanLock = _allCanRemove = false;
	}

	void signalChanged (int index, Package *package)
	{
		inited = false;
		PkgList list (this); refcount++;
		for (std::list <PkgList::Listener *>::iterator it = listeners.begin();
		     it != listeners.end(); it++)
			(*it)->entryChanged (list, index, package);
	}

	void signalInserted (int index, Package *package)
	{
		inited = false;
		PkgList list (this); refcount++;
		for (std::list <PkgList::Listener *>::iterator it = listeners.begin();
		     it != listeners.end(); it++)
			(*it)->entryInserted (list, index, package);
	}

	void signalDeleted (int index, Package *package)
	{
		inited = false;
		PkgList list (this); refcount++;
		for (std::list <PkgList::Listener *>::iterator it = listeners.begin();
		     it != listeners.end(); it++)
			(*it)->entryDeleted (list, index, package);
	}

	// Ypp callback
	void packageModified (Package *package)
	{
		bool live = liveList();
		if (!live && listeners.empty())
			return;

		bool match = this->match (package);
		int index = find (package);
		if (live) {
			if (index >= 0) {  // is on the pool
				if (match)  // modified
					signalChanged (index, package);
				else {  // removed
					signalDeleted (index, package);
					remove (index);
				}
			}
			else {  // not on pool
				if (match) {  // inserted
					pool.push_back (package);
					signalInserted (pool.size()-1, package);
				}
			}
		}
		else if (index >= 0)
			signalChanged (index, package);
	}

	virtual void entryChanged  (const PkgList list, int index, Package *package)
	{ packageModified (package); }
	virtual void entryInserted (const PkgList list, int index, Package *package)
	{ packageModified (package); }
	virtual void entryDeleted  (const PkgList list, int index, Package *package)
	{ packageModified (package); }

	// for sub-classes to implement live lists
	virtual bool liveList() const { return false; }
	virtual bool match (Package *pkg) const { return true; }
	virtual bool highlight (Package *pkg) const { return false; }
};

Ypp::PkgList::PkgList()
: impl (new PkgList::Impl (NULL))
{}

Ypp::PkgList::PkgList (Ypp::PkgList::Impl *impl)  // sub-class
: impl (impl)
{}

Ypp::PkgList::PkgList (const Ypp::PkgList &other)
: impl (other.impl)
{ impl->refcount++; }

Ypp::PkgList & Ypp::PkgList::operator = (const Ypp::PkgList &other)
{
	if (--impl->refcount <= 0) delete impl;
	impl = other.impl;
	impl->refcount++;
	return *this;
}

Ypp::PkgList::~PkgList()
{ if (--impl->refcount <= 0) delete impl; }

Ypp::Package *Ypp::PkgList::get (int i) const
{ return i >= (signed) impl->pool.size() ? NULL : impl->pool.at (i); }

bool Ypp::PkgList::highlight (Ypp::Package *pkg) const
{ return impl->highlight (pkg); }

int Ypp::PkgList::size() const
{ return impl->pool.size(); }

void Ypp::PkgList::reserve (int size)
{ impl->pool.reserve (size); }

void Ypp::PkgList::append (Ypp::Package *package)
{ impl->pool.push_back (package); }

static bool pkgorder (Ypp::Package *a, Ypp::Package *b)
{ return strcasecmp (a->name().c_str(), b->name().c_str()) < 0; }

void Ypp::PkgList::sort (bool (* compare) (Package *, Package *))
{ std::sort (impl->pool.begin(), impl->pool.end(), compare ? compare : pkgorder); }

void Ypp::PkgList::remove (int i)
{ impl->remove (i); }

void Ypp::PkgList::copy (const Ypp::PkgList list)
{
	bool showProgress = false;
	GTimeVal then;
	g_get_current_time (&then);

	for (int i = 0; i < list.size(); i++) {
		Package *pkg = list.get (i);
		if (impl->match (pkg))
			append (pkg);

		if (showProgress) {
			if ((i % 500) == 0)
				ypp->impl->interface->loading (i / (float) list.size());
		}
		else if (i == 499) {  // show progress if it takes more than 1 sec to drive 'N' loops
			GTimeVal now;
			g_get_current_time (&now);
			if (now.tv_usec - then.tv_usec >= 35*1000 || now.tv_sec - then.tv_sec >= 1) {
				showProgress = true;
				ypp->impl->interface->loading (0);
			}
		}
	}
	if (showProgress)
		ypp->impl->interface->loading (1);
}

bool Ypp::PkgList::contains (const Ypp::Package *package) const
{ return find (package) != -1; }

int Ypp::PkgList::find (const Ypp::Package *package) const
{ return impl->find (package); }

Ypp::Package *Ypp::PkgList::find (const std::string &name) const
{
	for (int i = 0; i < size(); i++)
		if (name == get (i)->name())
			return get (i);
	return NULL;
}

bool Ypp::PkgList::operator == (const Ypp::PkgList &other) const
{ return this->impl == other.impl; }

bool Ypp::PkgList::installed() const
{ impl->buildProps(); return impl->_allInstalled; }

bool Ypp::PkgList::notInstalled() const
{ impl->buildProps(); return impl->_allNotInstalled; }

bool Ypp::PkgList::upgradable() const
{ impl->buildProps(); return impl->_allUpgradable; }

bool Ypp::PkgList::modified() const
{ impl->buildProps(); return impl->_allModified; }

bool Ypp::PkgList::locked() const
{ impl->buildProps(); return impl->_allLocked; }

bool Ypp::PkgList::unlocked() const
{ impl->buildProps(); return impl->_allUnlocked; }

bool Ypp::PkgList::canLock() const
{ impl->buildProps(); return impl->_allCanLock; }

bool Ypp::PkgList::canRemove() const
{ impl->buildProps(); return impl->_allCanRemove; }

void Ypp::PkgList::install()
{
	Ypp::get()->startTransactions();
	for (int i = 0; get (i); i++)
		get (i)->install (0);
	Ypp::get()->finishTransactions();
}

void Ypp::PkgList::remove()
{
	Ypp::get()->startTransactions();
	for (int i = 0; get (i); i++)
		get (i)->remove();
	Ypp::get()->finishTransactions();
}

void Ypp::PkgList::lock (bool toLock)
{
	Ypp::get()->startTransactions();
	for (int i = 0; get (i); i++)
		get (i)->lock (toLock);
	Ypp::get()->finishTransactions();
}

void Ypp::PkgList::undo()
{
	Ypp::get()->startTransactions();
	for (int i = 0; get (i); i++)
		get (i)->undo();
	Ypp::get()->finishTransactions();
}

void Ypp::PkgList::addListener (Ypp::PkgList::Listener *listener) const
{ const_cast <Impl *> (impl)->addListener (listener); }

void Ypp::PkgList::removeListener (Ypp::PkgList::Listener *listener) const
{ const_cast <Impl *> (impl)->removeListener (listener); }

//** PkgQuery

struct Ypp::PkgQuery::Query::Impl
{
	template <typename T>
	struct Key
	{
		Key() : defined (false) {}
		void set (T v)
		{
			defined = true;
			value = v;
		}
		bool is (const T &v) const
		{
			if (!defined) return true;
			return value == v;
		}
		bool defined;
		T value;
	};

	template <typename T>
	struct Keys
	{
		Keys() : defined (false) {}
		void add (T v)
		{
			defined = true;
			values.push_back (v);
		}
		bool is (const T &v) const
		{
			if (!defined) return true;
			typename std::list <T>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++)
				if (*it == v)
					return true;
			return false;
		}
		bool defined;
		std::list <T> values;
	};

	Keys <std::string> names;
	unsigned int use_name : 1, use_summary : 1, use_description : 1, use_filelist : 1,
	    use_authors : 1, whole_word : 1, whole_string : 1;
	Keys <Node *> categories, categories2;
	Keys <const Package *> collections;
	Keys <const Repository *> repositories;
	Key <bool> isInstalled, hasUpgrade, toModify, toInstall, toRemove;
	Key <bool> isRecommended, isSuggested;
	Key <int>  buildAge;
	Key <bool> isSupported;
	Key <int> severity;
	bool clear;
	Ypp::Package *highlight;

#if 0
	template <typename T>
	struct PropKey {
		T property;
		T value;
	};
	std::list <PropKey <bool> > boolKeys;
	std::list <PropKey <int > > intKeys;
	std::list <PropKey <std::string > > strKeys;
#endif

	Impl() : clear (false), highlight (NULL)
	{}

	bool match (Package *package) const
	{
		struct inner {
			static bool strstr (const char *str1, const char *str2,
			                    bool case_sensitive, bool whole_word, bool whole_string)
			{
				const char *i;
				if (whole_string) {
					if (case_sensitive)
						return strcmp (str1, str2) == 0;
					return strcasecmp (str1, str2) == 0;
				}
				if (case_sensitive)
					i = ::strstr (str1, str2);
				else
					i = ::strcasestr (str1, str2);
				if (whole_word && i) {  // check boundries
					if (i != str1 && isalpha (i[-1]))
						return false;
					int len = strlen (str2);
					if (i [len] && isalpha (i [len]))
						return false;
					return true;
				}
				return i;
			}
		};

		if (clear)
			return false;
		bool match = true;
#if 0
		for (std::list <PropKey <bool> >::const_iterator it = boolKeys.begin();
		     it != boolKeys.end(); it++) {
			const PropKey <bool> &key = *it;
			if (package->getPropertyBool (key.property) != key.value)
				return false;
		}
		for (std::list <PropKey <int> >::const_iterator it = intKeys.begin();
		     it != intKeys.end(); it++) {
			const PropKey <int> &key = *it;
			if (package->getPropertyInt (key.property) != key.value)
				return false;
		}
		bool str_match = false;
		for (std::list <PropKey <std::string> >::const_iterator it = strKeys.begin();
		     it != strKeys.end(); it++) {
			const PropKey <std::string> &key = *it;
			std::string pkg_value = package->getPropertyStr (key.property);
			if (pkg_value != key.value)
				return false;
		}
#endif
		if (match && (isInstalled.defined || hasUpgrade.defined)) {
			// only one of the specified status must match
			bool status_match = false;
			if (isInstalled.defined)
				status_match = isInstalled.is (package->isInstalled());
			if (!status_match && hasUpgrade.defined)
				status_match = hasUpgrade.is (package->hasUpgrade());
			match = status_match;
		}
		if (match && toModify.defined)
			match = toModify.is (package->toModify());
		if (match && toInstall.defined)
			match = toInstall.is (package->toInstall());
		if (match && toRemove.defined)
			match = toRemove.is (package->toRemove());
		if (match && isRecommended.defined)
			match = isRecommended.is (package->isRecommended());
		if (match && isSuggested.defined)
			match = isSuggested.is (package->isSuggested());
		if (match && buildAge.defined) {
			int age = package->buildAge();
			if (age < 0 || age > buildAge.value)
				match = false;
		}
		if (match && isSupported.defined)
			match = isSupported.is (package->isSupported());
		if (match && severity.defined)
			match = severity.is (package->severity());
		if (match && names.defined) {
			const std::list <std::string> &values = names.values;
			std::list <std::string>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++) {
				const char *key = it->c_str();
				bool str_match = false;
				if (use_name)
					str_match = inner::strstr (package->name().c_str(), key,
					                           false, whole_word, whole_string);
				if (!str_match && use_summary)
					str_match = inner::strstr (package->summary().c_str(), key,
					                           false, whole_word, whole_string);
				if (!str_match && use_description)
					str_match = inner::strstr (package->description (NO_MARKUP).c_str(), key,
					                           false, whole_word, whole_string);
				if (!str_match && use_filelist)
					str_match = inner::strstr (package->filelist (NO_MARKUP).c_str(), key,
					                           false, whole_word, whole_string);
				if (!str_match && use_authors)
					str_match = inner::strstr (package->authors (NO_MARKUP).c_str(), key,
					                           false, whole_word, whole_string);
				if (!str_match) {
					match = false;
					break;
				}
			}
			if (match && !highlight && use_name) {
				if (values.size() == 1 && !strcasecmp (values.front().c_str(), package->name().c_str()))
					const_cast <Impl *> (this)->highlight = package;
			}
		}
		if (match && categories.defined) {
			Ypp::Node *pkg_category = package->category();
			const std::list <Ypp::Node *> &values = categories.values;
			std::list <Ypp::Node *>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++) {
				GNode *node = (GNode *) (*it)->impl;
				if (g_node_find (node, G_PRE_ORDER, G_TRAVERSE_ALL, pkg_category))
					break;
			}
			match = it != values.end();
		}
		if (match && categories2.defined) {
			Ypp::Node *pkg_category = package->category2();
			const std::list <Ypp::Node *> &values = categories2.values;
			std::list <Ypp::Node *>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++) {
				GNode *node = (GNode *) (*it)->impl;
				if (g_node_find (node, G_PRE_ORDER, G_TRAVERSE_ALL, pkg_category))
					break;
			}
			match = it != values.end();
		}
		if (match && repositories.defined) {
			const std::list <const Repository *> &values = repositories.values;
			std::list <const Repository *>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++) {
				bool match = false;
				for (int i = 0; package->getAvailableVersion (i); i++) {
					const Ypp::Package::Version *version = package->getAvailableVersion (i);
					if (version->repo == *it) {
						// filter if available isn't upgrade
						if (package->isInstalled() && hasUpgrade.defined && hasUpgrade.value) {
							if (version->cmp > 0)
								match = true;
						}
						else
							match = true;
						break;
					}
				}
				if (match)
					break;
			}
			match = it != values.end();
		}
		if (match && collections.defined) {
			const std::list <const Ypp::Package *> &values = collections.values;
			std::list <const Ypp::Package *>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++)
				if (package->fromCollection (*it))
					break;
			match = it != values.end();
		}
		return match;
	}
};

Ypp::PkgQuery::Query::Query()
{ impl = new Impl(); }
Ypp::PkgQuery::Query::~Query()
{ delete impl; }

void Ypp::PkgQuery::Query::addNames (std::string value, char separator, bool use_name,
	bool use_summary, bool use_description, bool use_filelist, bool use_authors,
	bool whole_word, bool whole_string)
{
	if (separator) {
		const gchar delimiter[2] = { separator, '\0' };
		gchar **names = g_strsplit (value.c_str(), delimiter, -1);
		for (gchar **i = names; *i; i++)
			impl->names.add (*i);
		g_strfreev (names);
	}
	else
		impl->names.add (value);
	impl->use_name = use_name;
	impl->use_summary = use_summary;
	impl->use_description = use_description;
	impl->use_filelist = use_filelist;
	impl->use_authors = use_authors;
	impl->whole_word = whole_word;
	impl->whole_string = whole_string;
}
void Ypp::PkgQuery::Query::addCategory (Ypp::Node *value)
{ impl->categories.add (value); }
void Ypp::PkgQuery::Query::addCategory2 (Ypp::Node *value)
{ impl->categories2.add (value); }
void Ypp::PkgQuery::Query::addCollection (const Ypp::Package *value)
{ impl->collections.add (value); }
void Ypp::PkgQuery::Query::addRepository (const Repository *value)
{ impl->repositories.add (value); }
void Ypp::PkgQuery::Query::setIsInstalled (bool value)
{ impl->isInstalled.set (value); }
void Ypp::PkgQuery::Query::setHasUpgrade (bool value)
{ impl->hasUpgrade.set (value); }
void Ypp::PkgQuery::Query::setToModify (bool value)
{ impl->toModify.set (value); }
void Ypp::PkgQuery::Query::setToInstall (bool value)
{ impl->toInstall.set (value); }
void Ypp::PkgQuery::Query::setToRemove (bool value)
{ impl->toRemove.set (value); }
void Ypp::PkgQuery::Query::setIsRecommended (bool value)
{ impl->isRecommended.set (value); }
void Ypp::PkgQuery::Query::setIsSuggested (bool value)
{ impl->isSuggested.set (value); }
void Ypp::PkgQuery::Query::setBuildAge (int value)
{ impl->buildAge.set (value); }
void Ypp::PkgQuery::Query::setIsSupported (bool value)
{ impl->isSupported.set (value); }
void Ypp::PkgQuery::Query::setSeverity (int value)
{ impl->severity.set (value); }
void Ypp::PkgQuery::Query::setClear()
{ impl->clear = true; }

struct Ypp::PkgQuery::Impl : public Ypp::PkgList::Impl
{
Query *query;

	Impl (const PkgList parent, Query *query)
	: PkgList::Impl (&parent), query (query)
	{}

	~Impl()
	{ delete query; }

	virtual bool liveList() const
	{ return true; }

	virtual bool match (Package *pkg) const
	{ return query ? query->impl->match (pkg) : true; }

	virtual bool highlight (Package *pkg) const
	{
		if (query && query->impl->highlight == pkg)
			return true;
		return parent->highlight (pkg);  // this might be a sub-query
	}
};

Ypp::PkgQuery::PkgQuery (const Ypp::PkgList list, Ypp::PkgQuery::Query *query)
: PkgList ((impl = new PkgQuery::Impl (list, query)))
{ copy (list); }

Ypp::PkgQuery::PkgQuery (Ypp::Package::Type type, Ypp::PkgQuery::Query *query)
: PkgList ((impl = new PkgQuery::Impl (Ypp::get()->getPackages (type), query)))
{ copy (Ypp::get()->getPackages (type)); }

//** PkgSort

static std::string _prop;  // hack
static bool _ascend;

static bool prop_order (Ypp::Package *a, Ypp::Package *b)
{
	int r = strcasecmp (a->getPropertyStr (_prop).c_str(), b->getPropertyStr (_prop).c_str());
	return _ascend ? r < 0 : r >= 0;
}

Ypp::PkgSort::PkgSort (PkgList list, const std::string &prop, bool ascend)
: PkgList()
{ copy (list); _prop = prop; _ascend = ascend; sort (prop_order); }

//** Misc

struct Ypp::Disk::Impl
{
std::list <Ypp::Disk::Listener *> listeners;
std::vector <Disk::Partition *> partitions;

	Impl()
	: partitions (NULL)
	{
		if (zypp::getZYpp()->diskUsage().empty())
			zypp::getZYpp()->setPartitions (
			    zypp::DiskUsageCounter::detectMountPoints());
	}

	~Impl()
	{
		clearPartitions();
	}

	void packageModified()
	{
		clearPartitions();
		for (std::list <Ypp::Disk::Listener *>::iterator it = listeners.begin();
		     it != listeners.end(); it++)
			(*it)->updateDisk();
	}

	void clearPartitions()
	{
		for (unsigned int i = 0; i < partitions.size(); i++)
			delete partitions[i];
		partitions.clear();
	}

	const Partition *getPartition (int nb)
	{
		if (partitions.empty()) {
			typedef zypp::DiskUsageCounter::MountPoint    ZyppDu;
			typedef zypp::DiskUsageCounter::MountPointSet ZyppDuSet;
			typedef zypp::DiskUsageCounter::MountPointSet::iterator ZyppDuSetIterator;

			ZyppDuSet diskUsage = zypp::getZYpp()->diskUsage();
			partitions.reserve (diskUsage.size());
			for (ZyppDuSetIterator it = diskUsage.begin(); it != diskUsage.end(); it++) {
				const ZyppDu &point = *it;
				if (!point.readonly) {
					// partition fields: dir, used_size, total_size (size on Kb)
					Ypp::Disk::Partition *partition = new Partition();
					partition->path = point.dir;
					partition->used = point.pkg_size;
					partition->delta = point.pkg_size - point.used_size;
					partition->total = point.total_size;
					partition->used_str =
						zypp::ByteCount (partition->used, zypp::ByteCount::K).asString();
					partition->free_str =
						zypp::ByteCount (partition->total - partition->used,
							zypp::ByteCount::K).asString();
					partition->delta_str =
						zypp::ByteCount (partition->delta, zypp::ByteCount::K).asString();
					partition->total_str =
						zypp::ByteCount (partition->total, zypp::ByteCount::K).asString();
					partitions.push_back (partition);
				}
			}
		}
		if (nb >= (signed) partitions.size())
			return NULL;
		return partitions[nb];
	}
};

Ypp::Disk::Disk()
{ impl = new Impl(); }
Ypp::Disk::~Disk()
{ delete impl; }

void Ypp::Disk::addListener (Ypp::Disk::Listener *listener)
{ impl->listeners.push_back (listener);  }

const Ypp::Disk::Partition *Ypp::Disk::getPartition (int nb)
{ return impl->getPartition (nb); }

Ypp::Node *Ypp::getFirstCategory (Ypp::Package::Type type)
{
	impl->getPackages (type);
	if (impl->categories[type] != 0)
		return impl->categories[type]->getFirst();
	return NULL;
}

Ypp::Node *Ypp::getFirstCategory2 (Ypp::Package::Type type)
{
	impl->getPackages (type);
	return impl->categories2->getFirst();
}

Ypp::Node *Ypp::Node::next()
{
	GNode *ret = ((GNode *) impl)->next;
	if (ret) return (Ypp::Node *) ret->data;
	return NULL;
}

Ypp::Node *Ypp::Node::child()
{
	GNode *ret = ((GNode *) impl)->children;
	if (ret) return (Ypp::Node *) ret->data;
	return NULL;
}

Ypp::Node *Ypp::Impl::addCategory (Ypp::Package::Type type, const std::string &category_str, const std::string &order)
{
	struct inner {
		static int cmp (const char *a, const char *b)
		{
			// Other group should always go as last
			if (!strcmp (a, _("Other")))
				return !strcmp (b, _("Other")) ? 0 : 1;
			if (!strcmp (b, _("Other")))
				return -1;
			return strcasecmp (a, b);
//			return g_utf8_collate (a, b);  // slow?
		}
	};

	if (!categories[type]) {
		const char *trans_domain = 0;
		if (type == Package::PACKAGE_TYPE)
			trans_domain = "rpm-groups";
		categories[type] = new StringTree (inner::cmp, '/', trans_domain);
	}

	std::string category = category_str;
	if (category_str.empty())
		category = _("Other");
	return categories[type]->add (category, order);
}

void Ypp::Impl::polishCategories (Ypp::Package::Type type)
{
	// some treatment on categories
	// Packages must be on leaves. If not, create a "Other" leaf, and put it there.
	if (type == Package::PACKAGE_TYPE) {
		PkgList *list = ypp->impl->getPackages (type);
		for (int i = 0; list->get (i); i++) {
			Package *pkg = list->get (i);
			PackageSel *sel = (PackageSel *) pkg->impl;
			Ypp::Node *ynode = pkg->category();
			if (ynode->child()) {
				GNode *node = (GNode *) ynode->impl;
				GNode *last = g_node_last_child (node);
				if (((Ypp::Node *) last->data)->name == _("Other"))
					sel->m_category = (Ypp::Node *) last->data;
				else {
					// must create a "Other" node
					Ypp::Node *yN = new Ypp::Node();
					GNode *n = g_node_new ((void *) yN);
					yN->name = _("Other");
					yN->icon = NULL;
					yN->impl = (void *) n;
					g_node_insert_before (node, NULL, n);
					sel->m_category = yN;
				}
			}
		}
	}
}

//** Ypp top methods & internal connections

Ypp::Impl::Impl()
: favoriteRepo (NULL), disk (NULL), interface (NULL),
  pkg_listeners (NULL), inTransaction (false), transactions (NULL)
{
	for (int i = 0; i < Package::TOTAL_TYPES; i++) {
		packages[i] = NULL;
		categories[i] = NULL;
	}
	categories2 = NULL;
}

Ypp::Impl::~Impl()
{
	struct inner {
		static void free_repo (void *data, void *_data)
		{ delete ((Repository *) data); }
	};

	for (int t = 0; t < Package::TOTAL_TYPES; t++) {
		if (packages [t])
			for (int p = 0; p < packages[t]->size(); p++)
				delete packages[t]->get (p);
		delete packages [t];
		delete categories[t];
	}
	delete categories2;
	for (unsigned int i = 0; i < repos.size(); i++)
		delete repos[i];

	// don't delete pools as they don't actually belong to Ypp, just listeners
	g_slist_free (pkg_listeners);
	delete disk;
}

const Ypp::Repository *Ypp::Impl::getRepository (int nb)
{
	if (repos.empty()) {
		struct inner {
			static void addRepo (Ypp::Impl *impl, const zypp::RepoInfo &info)
			{
				Repository *repo = new Repository();
				repo->name = info.name();
				if (!info.baseUrlsEmpty())
					repo->url = info.baseUrlsBegin()->asString();
				repo->alias = info.alias();
				repo->enabled = info.enabled();
				impl->repos.push_back (repo);
			}
		};
		
		int size;
		zypp::RepoManager manager;
		std::list <zypp::RepoInfo> known_repos = manager.knownRepositories();
		size = known_repos.size();
		size += zyppPool().knownRepositoriesSize();
		repos.reserve (size);

		for (zypp::ResPoolProxy::repository_iterator it = zyppPool().knownRepositoriesBegin();
		     it != zyppPool().knownRepositoriesEnd(); it++) {
			const zypp::Repository &zrepo = *it;
			if (zrepo.isSystemRepo())
				continue;
			inner::addRepo (this, zrepo.info());
		}
		// zyppPool::knownRepositories is more accurate, but it doesn't feature disabled
		// repositories. Add them with the following API.
		for (std::list <zypp::RepoInfo>::const_iterator it = known_repos.begin();
		     it != known_repos.end(); it++) {
			const zypp::RepoInfo info = *it;
			if (info.enabled())
				continue;
			inner::addRepo (this, info);
		}
	}
	if (nb >= (signed) repos.size())
		return NULL;
	return repos[nb];
}

const Ypp::Repository *Ypp::Impl::getRepository (const std::string &alias)
{
	for (int i = 0; getRepository (i); i++)
		if (getRepository (i)->alias == alias)
			return getRepository (i);
	return NULL; /*error*/
}

zypp::RepoInfo Ypp::Impl::getRepoInfo (const Repository *repo)
{
	zypp::RepoManager manager;
	std::list <zypp::RepoInfo> zrepos = manager.knownRepositories();
	for (std::list <zypp::RepoInfo>::const_iterator it = zrepos.begin();
		 it != zrepos.end(); it++)
		if (repo->alias == it->alias())
			return *it;
	// error
	zypp::RepoInfo i;
	return i;
}

Ypp::Disk *Ypp::Impl::getDisk()
{
	if (!disk)
		disk = new Disk();
	return disk;
}

bool Ypp::Impl::acceptLicense (Ypp::Package *package, const std::string &license)
{
	if (interface)
		return interface->acceptLicense (package, license);
	return true;
}

void Ypp::Impl::notifyMessage (Ypp::Package *package, const std::string &message)
{
	if (interface)
		interface->notifyMessage (package, message);
}

Ypp::Problem::Solution *Ypp::Problem::getSolution (int nb)
{
	return (Ypp::Problem::Solution *) g_slist_nth_data ((GSList *) impl, nb);
}

bool Ypp::Impl::resolveProblems()
{
	zypp::Resolver_Ptr zResolver = zypp::getZYpp()->resolver();
	bool resolved = false;
	while (true) {
		if (zResolver->resolvePool()) {
			resolved = true;
			break;
		}
		zypp::ResolverProblemList zProblems = zResolver->problems();
		if ((resolved = zProblems.empty()))
			break;
		if (!interface)
			break;

		std::list <Problem *> problems;
		for (zypp::ResolverProblemList::iterator it = zProblems.begin();
			 it != zProblems.end(); it++) {
			Problem *problem = new Problem();
			problem->description = (*it)->description();
			problem->details = (*it)->details();
			GSList *solutions = NULL;
			zypp::ProblemSolutionList zSolutions = (*it)->solutions();
			for (zypp::ProblemSolutionList::iterator jt = zSolutions.begin();
				 jt != zSolutions.end(); jt++) {
				Problem::Solution *solution = new Problem::Solution();
				solution->description = (*jt)->description();
				solution->details = (*jt)->details();
				solution->apply = false;
				solution->impl = (void *) get_pointer (*jt);
				solutions = g_slist_append (solutions, solution);
			}
			problem->impl = (void *) solutions;
			problems.push_back (problem);
		}

		resolved = interface->resolveProblems (problems);

		if (resolved) {
			zypp::ProblemSolutionList choices;
			for (std::list <Problem *>::iterator it = problems.begin();
				 it != problems.end(); it++) {
				for (int i = 0; (*it)->getSolution (i); i++) {
					Problem::Solution *solution = (*it)->getSolution (i);
					if (resolved && solution->apply)
						choices.push_back ((zypp::ProblemSolution *) solution->impl);
					delete solution;
				}
				delete *it;
			}

			zResolver->applySolutions (choices);
		}
		else
			break;
	}

	if (!resolved)
		zResolver->undo();
	return resolved;
}

void Ypp::Impl::packageModified (Ypp::Package *package)
{
	if (!g_slist_find (transactions, package)) /* could be a result of undo */
		transactions = g_slist_append (transactions, package);
	if (!inTransaction)
		finishTransactions();
}

void Ypp::Impl::startTransactions()
{
	inTransaction = true;
}

void Ypp::Impl::finishTransactions()
{
	inTransaction = true;
	bool cancel = (resolveProblems() == false);
	// check if any package was modified from a restricted repo
	if (!cancel && ypp->favoriteRepository()) {
		const Repository *repo = ypp->favoriteRepository();
		PkgList confirmPkgs;
		const PkgList *list = packages [Ypp::Package::PACKAGE_TYPE];
		for (int i = 0; list->get (i); i++) {
			Ypp::Package *pkg = list->get (i);
			if (pkg->impl->isTouched()) {
				const Ypp::Package::Version *version = 0;
				if (pkg->toInstall (&version)) {
					if (version->repo != repo)
						confirmPkgs.append (pkg);
				}
			}
		}
		if (confirmPkgs.size() != 0)
			cancel = (interface->allowRestrictedRepo (confirmPkgs) == false);
	}

	interface->loading (0);
	if (cancel) {
		// user canceled resolver -- undo all
		for (GSList *i = transactions; i; i = i->next)
			((Ypp::Package *) i->data)->undo();
	}
	else {
		// resolver won't tell us what changed -- tell pools about Auto packages
		// notify pools by the following order: 1st user selected, then autos (dependencies)
		for (int order = 0; order < 2; order++)
			for (int type = 0; type < Ypp::Package::TOTAL_TYPES; type++) {
				const PkgList *list = packages [type];
				if (list)
					for (int i = 0; i < list->size(); i++) {
						Ypp::Package *pkg = list->get (i);
						if (pkg->impl->isTouched()) {
							bool isAuto = pkg->isAuto();
							if ((order == 0 && !isAuto) || (order == 1 && isAuto)) {
								list->impl->signalChanged (i, pkg);
								pkg->impl->setNotTouched();
							}
						}
#if 0
						if ((i % 500) == 0) {
							float total = 2*Ypp::Package::TOTAL_TYPES*list->size();
							float progress = (i*order*type) / total;
							interface->loading (progress);
						}
#endif
					}
			}
	}
	g_slist_free (transactions);
	transactions = NULL;
	inTransaction = false;
	if (disk)
		disk->impl->packageModified();
	interface->loading (1);
}

Ypp::Ypp()
{
	impl = new Impl();
    zyppPool().saveState <zypp::Package>();
    zyppPool().saveState <zypp::Pattern>();
    zyppPool().saveState <zypp::Patch  >();
}

Ypp::~Ypp()
{
	setFavoriteRepository (NULL);
	delete impl;
}

const Ypp::Repository *Ypp::getRepository (int nb)
{ return impl->getRepository (nb); }

void Ypp::setFavoriteRepository (const Ypp::Repository *repo)
{
	if (impl->favoriteRepo) {
		zypp::RepoInfo info = impl->getRepoInfo (impl->favoriteRepo);
		info.setPriority (impl->favoriteRepoPriority);
	}

	impl->favoriteRepo = repo;

	if (repo) {
		zypp::RepoInfo info = impl->getRepoInfo (repo);
		impl->favoriteRepoPriority = info.priority();
		info.setPriority (1);
	}
}

const Ypp::Repository *Ypp::favoriteRepository()
{ return impl->favoriteRepo; }

Ypp::Disk *Ypp::getDisk()
{ return impl->getDisk(); }

void Ypp::setInterface (Ypp::Interface *interface)
{
	impl->interface = interface;
	// run the solver at start to check for problems
	impl->resolveProblems();
}

const Ypp::PkgList Ypp::getPackages (Ypp::Package::Type type)
{ return *impl->getPackages (type); }

bool Ypp::isModified()
{
	return zyppPool().diffState<zypp::Package>() ||
	       zyppPool().diffState<zypp::Pattern>() ||
	       zyppPool().diffState<zypp::Patch  >();
}

void Ypp::startTransactions()
{ impl->startTransactions(); }

void Ypp::finishTransactions()
{ impl->finishTransactions(); }

#include <fstream>
#include <zypp/SysContent.h>

bool Ypp::importList (const char *filename)
{
	bool ret = true;
	startTransactions();
	try {
		std::ifstream importFile (filename);
		zypp::syscontent::Reader reader (importFile);

		for (zypp::syscontent::Reader::const_iterator it = reader.begin();
		     it != reader.end(); it++) {
			std::string kind = it->kind(), name = it->name();

			Package::Type type = Ypp::Package::PACKAGE_TYPE;
			if (kind == "pattern")  // else "package"
				type = Ypp::Package::PATTERN_TYPE;

			Ypp::Package *pkg = getPackages (type).find (name);
			if (pkg && !pkg->isInstalled())
				pkg->install (0);
		}
	}
	catch (const zypp::Exception &exception) {
		ret = false;
	}
	finishTransactions();
	return ret;
}

bool Ypp::exportList (const char *filename)
{
	bool ret = true;
	zypp::syscontent::Writer writer;
	const zypp::ResPool &pool = zypp::getZYpp()->pool();

	// from yast-qt:
	for_each (pool.begin(), pool.end(),
		boost::bind (&zypp::syscontent::Writer::addIf, boost::ref (writer), _1));

	try {
		std::ofstream exportFile (filename);
		exportFile.exceptions (std::ios_base::badbit | std::ios_base::failbit);
	    exportFile << writer;

		yuiMilestone() << "Package list exported to " << filename << endl;
	}
	catch (std::exception &exception) {
		ret = false;
	}
	return ret;
}

bool Ypp::createSolverTestcase (const char *dirname)
{
	bool success;
	yuiMilestone() << "Generating solver test case START" << endl;
	success = zypp::getZYpp()->resolver()->createSolverTestcase (dirname);
	yuiMilestone() << "Generating solver test case END" << endl;
	return success;
}

