/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/*
  Textdomain "yast2-gtk"
 */

/* Ypp, zypp wrapper */
// check the header file for information about this wrapper

#include <config.h>
#include "YGi18n.h"
#include "yzyppwrapper.h"
#include "yzypptags.h"
#include <string.h>
#include <string>
#include <sstream>
#define YUILogComponent "gtk-pkg"
#include <YUILog.h>

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
	GNode *root;

	StringTree (Compare compare, char delim)
	: compare (compare), delim (delim)
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

	Ypp::Node *add (const std::string &tree_str, const char *icon)
	{
		const gchar delimiter[2] = { delim, '\0' };
		gchar **nodes_str = g_strsplit (tree_str.c_str(), delimiter, -1);

		GNode *parent = root, *sibling = 0;
		Ypp::Node *ret = 0;
		gchar **i;
		for (i = nodes_str; *i; i++) {
			if (!**i)
				continue;
			bool found = false;
			for (sibling = parent->children; sibling; sibling = sibling->next) {
				Ypp::Node *yNode = (Ypp::Node *) sibling->data;
				int cmp = (*compare) (*i, yNode->name.c_str());
				if (cmp == 0) {
					found = true;
					ret = yNode;
					break;
				}
				else if (cmp < 0)
					break;
			}
			if (!found)
				break;
			parent = sibling;
		}

		for (; *i; i++) {
			Ypp::Node *yNode = new Ypp::Node();
			GNode *n = g_node_new ((void *) yNode);
			yNode->name = *i;
			yNode->icon = icon;
			yNode->impl = (void *) n;
			g_node_insert_before (parent, sibling, n);
			parent = n;
			sibling = NULL;
			ret = yNode;
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
	GSList *getPackages (Package::Type type);

private:
	bool resolveProblems();
	Node *addCategory (Ypp::Package::Type type, const std::string &str);
	void polishCategories (Ypp::Package::Type type);
	Node *addCategory2 (Ypp::Package::Type type, ZyppSelectable sel);

	void startTransactions();
	void finishTransactions();

	friend class Ypp;
	GSList *packages [Package::TOTAL_TYPES];  // primitive pools
	StringTree *categories [Package::TOTAL_TYPES], *categories2;
	GSList *repos;
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

	virtual std::string description (bool rich) = 0;
	virtual std::string filelist (bool rich) { return ""; }
	virtual std::string changelog()          { return ""; }
	virtual std::string authors (bool rich)  { return ""; }
	virtual std::string support (bool rich)  { return ""; }
	virtual std::string icon() = 0;
	virtual bool isRecommended() const       { return false; }
	virtual bool isSuggested() const         { return false; }
	virtual int  buildAge() const            { return 0; }
	virtual bool isUnsupported() const       { return false; }

	virtual std::string provides (bool rich) const { return ""; }
	virtual std::string requires (bool rich) const { return ""; }

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

std::string Ypp::Package::description (bool rich) { return impl->description (rich); }
std::string Ypp::Package::filelist (bool rich)    { return impl->filelist (rich); }
std::string Ypp::Package::changelog()             { return impl->changelog(); }
std::string Ypp::Package::authors (bool rich)     { return impl->authors (rich); }
std::string Ypp::Package::support (bool rich)     { return impl->support (rich); }
std::string Ypp::Package::icon()                  { return impl->icon(); }
bool Ypp::Package::isRecommended() const          { return impl->isRecommended(); }
bool Ypp::Package::isSuggested() const            { return impl->isSuggested(); }
int Ypp::Package::buildAge() const                { return impl->buildAge(); }
bool Ypp::Package::isUnsupported() const          { return impl->isUnsupported(); }

std::string Ypp::Package::provides (bool rich) const { return impl->provides (rich); }
std::string Ypp::Package::requires (bool rich) const { return impl->requires (rich); }

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
		if (type == Ypp::Package::PATTERN_TYPE)
			m_isInstalled = m_sel->candidateObj().isSatisfied();
		else {
			if (m_sel->installedEmpty())
				m_isInstalled = false;
			else
				m_isInstalled = !m_sel->installedObj().isBroken();
		}
		// don't use getAvailableVersion(0) for hasUpgrade() has its inneficient.
		// let's just cache candidate() at start, which should point to the newest version.
		m_hasUpgrade = false;
		ZyppObject candidate = sel->candidateObj();
		ZyppObject installed = sel->installedObj();
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
			if (type == Ypp::Package::PATTERN_TYPE) {
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

	virtual std::string description (bool rich)
	{
		ZyppObject object = m_sel->theObj();
		std::string text = object->description(), br = "<br>";
		if (!rich)
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

					YGUtils::escapeMarkup (text);
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
				std::string url = package->url(), license = package->license();
				if (!url.empty())
					text += br + "<b>" + _("Website:") + "</b> <a href=\"" + url + "\">" + url + "</a>";
				if (!license.empty())
					text += br + "<b>" + _("License:") + "</b> " + license;
#if ZYPP_VERSION >= 5013001
				text += br + "<b>" + _("Size:") + "</b> " + object->installSize().asString();
#else
				text += br + "<b>" + _("Size:") + "</b> " + object->installsize().asString();
#endif

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
					text += std::string ("<b>") + _("Last build from:") + "</b> " + m_sel->candidateObj()->buildtime().form("%x");
				break;
			}
			case Ypp::Package::PATCH_TYPE:
			{
				ZyppPatch patch = tryCastToZyppPatch (object);
				if (patch->rebootSuggested())
					text += br + br + "<b>" + _("Reboot needed!") + "</b>";
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
				stream << "\n\n" << _("Installed: ") << installed << _(" of ") << total;
				text += stream.str();
				break;
			}
			default:
				break;
		}
		return text;
	}

	virtual std::string filelist (bool rich)
	{
		std::string text;
		ZyppObject object = m_sel->installedObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		if (package) {
			if (rich) {
				StringTree tree (strcmp, '/');

				const std::list <std::string> &filesList = package->filenames();
				for (std::list <std::string>::const_iterator it = filesList.begin();
					 it != filesList.end(); it++)
					tree.add (*it, 0);

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
				const std::list <std::string> &filesList = package->filenames();
				for (std::list <std::string>::const_iterator it = filesList.begin();
					 it != filesList.end(); it++)
					text += *it + " ";
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
				YGUtils::escapeMarkup (author);
				YGUtils::escapeMarkup (changes);
				YGUtils::replace (changes, "\n", 1, "<br>");
				if (author.compare (0, 2, "- ", 2) == 0)  // zypp returns a lot of author strings as
					author.erase (0, 2);                  // "- author". wtf?
				text += date + " (" + author + "):<br><blockquote>" + changes + "</blockquote>";
			}
		}
		return text;
	}

	virtual std::string authors (bool rich)
	{
		std::string text;
		ZyppObject object = m_sel->theObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		if (package) {
			std::string packager = package->packager(), vendor = package->vendor(), authors;
			YGUtils::escapeMarkup (packager);
			YGUtils::escapeMarkup (vendor);
			const std::list <std::string> &authorsList = package->authors();
			for (std::list <std::string>::const_iterator it = authorsList.begin();
				 it != authorsList.end(); it++) {
				std::string author (*it);
				if (rich)
					YGUtils::escapeMarkup (author);
				if (!authors.empty()) {
					if (rich)
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
				if (rich) {
					YGUtils::escapeMarkup (str);
					YGUtils::replace (str, "\n", 1, "<br>");
				}
				authors += str;
			}
			if (rich) {
				if (!authors.empty())
					text += _("Developed by:") + ("<blockquote>" + authors) + "</blockquote>";
				if (!packager.empty() || (!vendor.empty() && !text.empty())) {
					text += _("Packaged by:");
					text += "<blockquote>";
					if (!packager.empty())
						text += packager;
					if (!vendor.empty()) {
						if (!packager.empty())
							text += "<br>(";
						text += vendor;
						if (!packager.empty())
							text += ")";
					}
					text += "</blockquote>";
				}
			}
			else
				return authors;
		}
		return text;
	}

	virtual std::string support (bool rich)
	{
		std::string text;
#if ZYPP_VERSION >= 5013001
		ZyppObject object = m_sel->theObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		if (package) {
			zypp::VendorSupportOption opt = package->vendorSupport();
			text = zypp::asUserString (opt) + ": ";
			std::string str (zypp::asUserStringDescription (opt));
			if (rich)
				YGUtils::escapeMarkup (str);
			text += str;
		}
#endif
		return text;
	}

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

	virtual bool isUnsupported() const
	{
#if ZYPP_VERSION >= 5013001
		if (type != Ypp::Package::PACKAGE_TYPE)
			return false;
		ZyppObject object = m_sel->theObj();
		ZyppPackage package = tryCastToZyppPkg (object);
		return package->maybeUnsupported();
#else
		return false;
#endif
	}

	virtual std::string provides (bool rich) const
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
		if (rich)
			YGUtils::escapeMarkup (text);
		return text;
	}

	virtual std::string requires (bool rich) const
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
		if (rich)
			YGUtils::escapeMarkup (text);
		return text;
	}

	virtual bool containsPackage (const Ypp::Package *package)
	{
		if (type == Ypp::Package::PATTERN_TYPE) {
			PackageSel *sel = (PackageSel *) package->impl;
			return g_slist_find (getContainedPackages(), get_pointer (sel->m_sel)) != NULL;
		}
		return NULL;
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
	{ return m_category2; }

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
	{
		return m_sel->toDelete();
	}

	virtual bool toModify()
	{
		return m_sel->toModify();
	}

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

	virtual bool canLock() { return true; }

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
			const ZyppObject installedObj = m_sel->installedObj();
			assert (installedObj != NULL);
			m_installedVersion = constructVersion (installedObj, NULL);
		}
		return m_installedVersion;
	}

	virtual const Ypp::Package::Version *getAvailableVersion (int nb)
	{
		if (!m_availableVersions) {
			const ZyppObject installedObj = m_sel->installedObj();
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

	virtual std::string description (bool rich)
	{
		std::string text ("(" + m_locale.code() + ")");
		int installed, total;
		containsStats (&installed, &total);
		std::ostringstream stream;
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

	virtual bool canLock() { return false; }
	virtual void lock (bool lock) {}
};

// Packages Factory
GSList *Ypp::Impl::getPackages (Ypp::Package::Type type)
{
	if (!packages[type]) {
		GSList *pool = NULL;
		struct inner {
			static gint compare (gconstpointer _a, gconstpointer _b)
			{
				Package *a = (Package *) _a;
				Package *b = (Package *) _b;
				return strcasecmp (a->name().c_str(), b->name().c_str());
			}
		};

		if (type == Package::LANGUAGE_TYPE) {
			zypp::LocaleSet locales = zypp::getZYpp()->pool().getAvailableLocales();
			for (zypp::LocaleSet::const_iterator it = locales.begin();
			     it != locales.end(); it++) {
				Package *package = new Package (new PackageLang (type, *it));
				pool = g_slist_prepend (pool, package);
			}
		}
		else {  // Pool Selectables
			ZyppPool::const_iterator it, end;
			switch (type) {
				case Package::PACKAGE_TYPE:
					it = zyppPool().byKindBegin <zypp::Package>();
					end = zyppPool().byKindEnd <zypp::Package>();
					break;
				case Package::PATTERN_TYPE:
					it = zyppPool().byKindBegin <zypp::Pattern>();
					end = zyppPool().byKindEnd <zypp::Pattern>();
					break;
				case Package::PATCH_TYPE:
					it = zyppPool().byKindBegin <zypp::Patch>();
					end = zyppPool().byKindEnd <zypp::Patch>();
					break;
				default:
					break;
			}
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
						category = addCategory (type, zpackage->group());
						category2 = addCategory2 (type, sel);
						break;
					}
					case Package::PATTERN_TYPE:
					{
						ZyppPattern pattern = tryCastToZyppPattern (object);
						if (!pattern || !pattern->userVisible())
							continue;
						category = addCategory (type, pattern->category());
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
							if (sel->installedEmpty() && sel->candidateObj().isSatisfied())
								continue;
						}
					continue;
						category = addCategory (type, patch->category());
						break;
					}
					default:
						break;
				}

				Package *package = new Package (new PackageSel (type, sel, category, category2));
				pool = g_slist_prepend (pool, package);
			}
		}
		// its faster to prepend then reverse, as we avoid iterating for each append
		// lets reverse before sorting, as they are somewhat sorted already
		pool = g_slist_reverse (pool);
		// a sort merge. Don't use g_slist_insert_sorted() -- its linear
		pool = g_slist_sort (pool, inner::compare);
		packages[type] = pool;
		polishCategories (type);
	}
	return packages[type];
}

//** Query

struct Ypp::QueryPool::Query::Impl
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

	Keys <Ypp::Package::Type> types;
	Keys <std::string> names;
	int use_name : 1, use_summary : 1, use_description : 1, use_filelist : 1,
	    use_authors : 1, full_word_match : 1;
	Keys <Node *> categories, categories2;
	Keys <const Package *> collections;
	Keys <const Repository *> repositories;
	Key <bool> isInstalled;
	Key <bool> hasUpgrade;
	Key <bool> toModify;
	Key <bool> isRecommended;
	Key <bool> isSuggested;
	Key <int>  buildAge;
	Key <bool> isUnsupported;
	bool clear;
	Ypp::Package *highlight;

	Impl()
	{
		highlight = NULL;
		clear = false;
	}

	bool match (Package *package)
	{
		struct inner {
			static bool strstr (const char *str1, const char *str2,
			                    bool case_sensitive, bool full_word_match)
			{
				const char *i;
				if (case_sensitive)
					i = ::strstr (str1, str2);
				else
					i = ::strcasestr (str1, str2);
				if (full_word_match && i) {  // check boundries
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
		if (match && types.defined)
			match = types.is (package->type());
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
		if (match && isRecommended.defined)
			match = isRecommended.is (package->isRecommended());
		if (match && isSuggested.defined)
			match = isSuggested.is (package->isSuggested());
		if (match && buildAge.defined) {
			int age = package->buildAge();
			if (age < 0 || age > buildAge.value)
				match = false;
		}
		if (match && isUnsupported.defined)
			match = isUnsupported.is (package->isUnsupported());
		if (match && names.defined) {
			const std::list <std::string> &values = names.values;
			std::list <std::string>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++) {
				const char *key = it->c_str();
				bool str_match = false;
				if (use_name)
					str_match = inner::strstr (package->name().c_str(), key,
					                           false, full_word_match);
				if (!str_match && use_summary)
					str_match = inner::strstr (package->summary().c_str(), key,
					                           false, full_word_match);
				if (!str_match && use_description)
					str_match = inner::strstr (package->description (false).c_str(), key,
					                           false, full_word_match);
				if (!str_match && use_filelist)
					str_match = inner::strstr (package->filelist (false).c_str(), key,
					                           false, full_word_match);
				if (!str_match && use_authors)
					str_match = inner::strstr (package->authors (false).c_str(), key,
					                           false, full_word_match);
				if (!str_match) {
					match = false;
					break;
				}
			}
			if (match && !highlight && use_name) {
				if (values.size() == 1 && !strcasecmp (values.front().c_str(), package->name().c_str()))
					highlight = package;
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

Ypp::QueryPool::Query::Query()
{ impl = new Impl(); }
Ypp::QueryPool::Query::~Query()
{ delete impl; }

void Ypp::QueryPool::Query::addType (Ypp::Package::Type value)
{ impl->types.add (value); }
void Ypp::QueryPool::Query::addNames (std::string value, char separator, bool use_name,
	bool use_summary, bool use_description, bool use_filelist, bool use_authors,
	bool full_word_match)
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
	impl->full_word_match = full_word_match;
}
void Ypp::QueryPool::Query::addCategory (Ypp::Node *value)
{ impl->categories.add (value); }
void Ypp::QueryPool::Query::addCategory2 (Ypp::Node *value)
{ impl->categories2.add (value); }
void Ypp::QueryPool::Query::addCollection (const Ypp::Package *value)
{ impl->collections.add (value); }
void Ypp::QueryPool::Query::addRepository (const Repository *value)
{ impl->repositories.add (value); }
void Ypp::QueryPool::Query::setIsInstalled (bool value)
{ impl->isInstalled.set (value); }
void Ypp::QueryPool::Query::setHasUpgrade (bool value)
{ impl->hasUpgrade.set (value); }
void Ypp::QueryPool::Query::setToModify (bool value)
{ impl->toModify.set (value); }
void Ypp::QueryPool::Query::setIsRecommended (bool value)
{ impl->isRecommended.set (value); }
void Ypp::QueryPool::Query::setIsSuggested (bool value)
{ impl->isSuggested.set (value); }
void Ypp::QueryPool::Query::setBuildAge (int value)
{ impl->buildAge.set (value); }
void Ypp::QueryPool::Query::setIsUnsupported (bool value)
{ impl->isUnsupported.set (value); }
void Ypp::QueryPool::Query::setClear()
{ impl->clear = true; }

//** Pool

struct Ypp::Pool::Impl : public Ypp::PkgListener
{
Pool::Listener *listener;

	Impl() : listener (NULL)
	{ ypp->addPkgListener (this); }
	virtual ~Impl()
	{ ypp->removePkgListener (this); }

	virtual bool matches (Package *package) = 0;

	virtual Iter find (Package *package) = 0;
	virtual Iter insert (Package *package) = 0;
	virtual void remove (Iter iter, Package *package) = 0;

	virtual void packageModified (Package *package)
	{
		bool match = matches (package);
		Iter iter = find (package);
		if (iter) {  // is on the pool
			if (match) {  // modified
				if (listener)
					listener->entryChanged (iter, package);
			}
			else {  // removed
				if (listener)
					listener->entryDeleted (iter, package);
				remove (iter, package);
			}
		}
		else {  // not on pool
			if (match) {  // inserted
				iter = insert (package);
				if (listener)
					listener->entryInserted (iter, package);
			}
		}
	}
};

Ypp::Pool::Pool (Ypp::Pool::Impl *impl)
: impl (impl)
{}
Ypp::Pool::~Pool()
{ delete impl; }

void Ypp::Pool::setListener (Ypp::Pool::Listener *listener)
{ impl->listener = listener; }

int Ypp::Pool::size()
{
	int size = 0;
	for (Iter iter = getFirst(); iter; iter = getNext (iter))
		size++;
	return size;
}

Ypp::Pool::Iter Ypp::Pool::fromPath (const Ypp::Pool::Path &path)
{
	Iter iter = 0;
	for (Path::const_iterator it = path.begin(); it != path.end(); it++) {
		if (iter == NULL)
			iter = getFirst();
		else
			iter = getChild (iter);
		int row = *it;
		for (int r = 0; r < row; r++)
			iter = getNext (iter);
	}
	return iter;
}

Ypp::Pool::Path Ypp::Pool::toPath (Ypp::Pool::Iter iter)
{
	Path path;
	Iter parent, sibling;
	bool done = false;
	while (!done) {
		parent = getParent (iter);
		if (parent)
			sibling = getChild (parent);
		else {
			sibling = getFirst();
			done = true;
		}
		int row = 0;
		for (; sibling != iter; sibling = getNext (sibling))
			row++;
		path.push_front (row);
		iter = parent;
	}
	return path;
}

//** QueryPool

struct Ypp::QueryPool::Impl : public Ypp::Pool::Impl
{
Query *query;
GSList *packages;

	Impl (Query *query, bool startEmpty)
	: Pool::Impl(), query (query), packages (NULL)
	{
		if (!startEmpty)
			packages = buildPool (query);
	}
	virtual ~Impl()
	{
		delete query;
		g_slist_free (packages);
	}

	virtual bool matches (Package *package)
	{
		return query->impl->match (package);
	}

	virtual Iter find (Package *package)
	{
		GSList *i;
		for (i = packages; i; i = i->next) {
			if (package == i->data)
				break;
		}
		return (Iter) i;
	}

	virtual Iter insert (Package *package)
	{
		packages = g_slist_append (packages, (gpointer) package);
		Iter iter = (Iter) g_slist_last (packages);
		return iter;
	}

	virtual void remove (Iter iter, Package *package)
	{
		packages = g_slist_delete_link (packages, (GSList *) iter);
	}

private:
	static GSList *buildPool (Query *query)  // at construction
	{
		GSList *pool = NULL;
		for (int t = 0; t < Ypp::Package::TOTAL_TYPES; t++) {
			if (!query->impl->types.is ((Ypp::Package::Type) t))
				continue;
			if (!query->impl->types.defined && t != Ypp::Package::PACKAGE_TYPE)
				continue;
			GSList *entire_pool = ypp->impl->getPackages ((Ypp::Package::Type) t);
			for (GSList *i = entire_pool; i; i = i->next) {
				Package *pkg = (Package *) i->data;
				if (query->impl->match (pkg))
					pool = g_slist_append (pool, i->data);
			}
		}
		return pool;
	}
};

Ypp::QueryPool::QueryPool (Query *query, bool startEmpty)
: Ypp::Pool ((impl = new Ypp::QueryPool::Impl (query, startEmpty)))
{}

Ypp::Pool::Iter Ypp::QueryPool::getFirst()
{ return impl->packages; }

Ypp::Pool::Iter Ypp::QueryPool::getNext (Ypp::Pool::Iter iter)
{ return ((GSList *) iter)->next; }

Ypp::Package *Ypp::QueryPool::get (Ypp::Pool::Iter iter)
{ return (Ypp::Package *) ((GSList *) iter)->data; }

bool Ypp::QueryPool::highlight (Ypp::Pool::Iter iter)
{ return impl->query->impl->highlight == get (iter); }

//** TreePool

struct Ypp::TreePool::Impl : public Ypp::Pool::Impl
{
Package::Type type;
GNode *root;

	Impl (Package::Type type)
	: Pool::Impl(), type (type)
	{
		root = g_node_new (NULL);
		buildPool (root);
	}
	virtual ~Impl()
	{
		g_node_destroy (root);
	}

	virtual bool matches (Package *package)
	{
		return package->type() == type;
	}

	virtual Iter find (Package *package)
	{
		return (Iter) g_node_find (root, G_PRE_ORDER, G_TRAVERSE_LEAVES, package);
	}

	virtual Iter insert (Package *package) { return 0; }
	virtual void remove (Iter iter, Package *package) {}

private:
	void buildPool (GNode *root)  // at construction
	{
		GSList *packages = ypp->impl->getPackages (type);
		Ypp::Node *node = NULL;  // null for first round
		GNode *cat = root;
		do {
			for (GSList *p = packages; p; p = p->next) {
				Ypp::Package *pkg = (Ypp::Package *) p->data;
				if (pkg->category() == node)
					g_node_append_data (cat, pkg);
			}
			if (node)
				node = node->next();
			else
				node = ypp->getFirstCategory (type);
			if (node)
				cat = g_node_append_data (root, node);
		} while (node);
	}
};

Ypp::TreePool::TreePool (Ypp::Package::Type type)
: Ypp::Pool ((impl = new Ypp::TreePool::Impl (type)))
{}

Ypp::Pool::Iter Ypp::TreePool::getFirst()
{ return impl->root->children; }

Ypp::Pool::Iter Ypp::TreePool::getNext (Ypp::Pool::Iter iter)
{ return ((GNode *) iter)->next; }

Ypp::Pool::Iter Ypp::TreePool::getParent (Ypp::Pool::Iter iter)
{
	GNode *parent = ((GNode *) iter)->parent;
	if (parent == impl->root)
		return NULL;
	return parent;
}

Ypp::Pool::Iter Ypp::TreePool::getChild (Ypp::Pool::Iter iter)
{ return ((GNode *) iter)->children; }

Ypp::Package *Ypp::TreePool::get (Ypp::Pool::Iter iter)
{
	GNode *node = (GNode *) iter;
	return node->children ? NULL : (Ypp::Package *) node->data;
}

std::string Ypp::TreePool::getName (Ypp::Pool::Iter iter)
{
	Ypp::Package *package = get (iter);
	if (package)
		return package->name();
	return ((Ypp::Node *) ((GNode *) iter)->data)->name;
}

//** Misc

struct Ypp::Disk::Impl
{
Ypp::Disk::Listener *listener;
GSList *partitions;

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
		if (listener)
			listener->update();
	}

	void clearPartitions()
	{
		for (GSList *i = partitions; i; i = i->next)
			delete ((Partition *) i->data);
		g_slist_free (partitions);
		partitions = NULL;
	}

	const Partition *getPartition (int nb)
	{
		if (!partitions) {
			typedef zypp::DiskUsageCounter::MountPoint    ZyppDu;
			typedef zypp::DiskUsageCounter::MountPointSet ZyppDuSet;
			typedef zypp::DiskUsageCounter::MountPointSet::iterator ZyppDuSetIterator;

			ZyppDuSet diskUsage = zypp::getZYpp()->diskUsage();
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
						zypp::ByteCount (partition->used, zypp::ByteCount::K).asString() + "B";
					partition->delta_str =
						zypp::ByteCount (partition->delta, zypp::ByteCount::K).asString() + "B";
					partition->total_str =
						zypp::ByteCount (partition->total, zypp::ByteCount::K).asString() + "B";
					partitions = g_slist_append (partitions, (gpointer) partition);
				}
			}
		}
		return (Partition *) g_slist_nth_data (partitions, nb);
	}
};

Ypp::Disk::Disk()
{ impl = new Impl(); }
Ypp::Disk::~Disk()
{ delete impl; }

void Ypp::Disk::setListener (Ypp::Disk::Listener *listener)
{ impl->listener = listener;  }

const Ypp::Disk::Partition *Ypp::Disk::getPartition (int nb)
{ return impl->getPartition (nb); }

Ypp::Package *Ypp::findPackage (Ypp::Package::Type type, const std::string &name)
{
	GSList *pool = ypp->impl->getPackages (type);
	for (GSList *i = pool; i; i = i->next) {
		Package *pkg = (Package *) i->data;
		if (pkg->name() == name)
			return pkg;
	}
	return NULL;
}

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

Ypp::Node *Ypp::Impl::addCategory (Ypp::Package::Type type, const std::string &category_str)
{
	struct inner {
		static int cmp (const char *a, const char *b)
		{
			// Other group should always go as last
			if (!strcmp (a, "Other"))
				return !strcmp (b, "Other") ? 0 : 1;
			if (!strcmp (b, "Other"))
				return -1;
			return strcmp (a, b);
		}
	};

	std::string category = category_str;
	if (type == Package::PATCH_TYPE) {
		if (category_str == "security")
			category = _("Security");
		else if (category_str == "recommended")
			category = _("Recommended");
		else if (category_str == "optional")
			category = _("Optional");
		else if (category_str == "yast")
			category = "YaST";
		else if (category_str == "document")
			category = _("Documentation");
	}

	if (!categories[type])
		categories[type] = new StringTree (inner::cmp, '/');
	return categories[type]->add (category, 0);
}

Ypp::Node *Ypp::Impl::addCategory2 (Ypp::Package::Type type, ZyppSelectable sel)
{
	struct inner {
		static int cmp (const char *a, const char *b)
		{
			int r = strcmp (a, b);
			if (r != 0) {
				const char *unknown = zypp_tag_group_enum_to_localised_text
					(PK_GROUP_ENUM_UNKNOWN);
				if (!strcmp (a, unknown))
					return 1;
				if (!strcmp (b, unknown))
					return -1;
			}
			return r;
		}
	};

	// different instances may be assigned different groups
	YPkgGroupEnum group = PK_GROUP_ENUM_UNKNOWN;
	for (int i = 0; i < 2; i++) {
		ZyppObject obj;
		if (i == 0)
			obj = sel->candidateObj();
		else
			obj = sel->installedObj();
		ZyppPackage pkg = tryCastToZyppPkg (obj);
		if (pkg) {
			group = zypp_tag_convert (pkg->group());
			if (group != PK_GROUP_ENUM_UNKNOWN)
				break;
		}
	}
	const char *group_name = zypp_tag_group_enum_to_localised_text (group);
	const char *group_icon = zypp_tag_enum_to_icon (group);

	if (!categories2)
		categories2 = new StringTree (inner::cmp, '/');
	return categories2->add (group_name, group_icon);
}

void Ypp::Impl::polishCategories (Ypp::Package::Type type)
{
	// some treatment on categories
	// Packages must be on leaves. If not, create a "Other" leaf, and put it there.
	if (type == Package::PACKAGE_TYPE) {
		GSList *pool = ypp->impl->getPackages (type);
		for (GSList *i = pool; i; i = i->next) {
			Package *pkg = (Package *) i->data;
			PackageSel *sel = (PackageSel *) pkg->impl;
			Ypp::Node *ynode = pkg->category();
			if (ynode->child()) {
				GNode *node = (GNode *) ynode->impl;
				GNode *last = g_node_last_child (node);
				if (((Ypp::Node *) last->data)->name == "Other")
					sel->m_category = (Ypp::Node *) last->data;
				else {
					// must create a "Other" node
					Ypp::Node *yN = new Ypp::Node();
					GNode *n = g_node_new ((void *) yN);
					yN->name = "Other";
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
: repos (NULL), favoriteRepo (NULL), disk (NULL), interface (NULL),
  pkg_listeners (NULL), inTransaction (false), transactions (NULL)
{
	for (int i = 0; i < Package::TOTAL_TYPES; i++) {
		packages[i] = NULL;
		categories[i] = NULL;
		categories2 = NULL;
	}
}

Ypp::Impl::~Impl()
{
	struct inner {
		static void free_package (void *data, void *_data)
		{ delete ((Package *) data); }
		static void free_repo (void *data, void *_data)
		{ delete ((Repository *) data); }
	};

	for (int t = 0; t < Package::TOTAL_TYPES; t++) {
		g_slist_foreach (packages[t], inner::free_package, NULL);
		g_slist_free (packages[t]);
		delete categories[t];
	}
	delete categories2;

	g_slist_foreach (repos, inner::free_repo, NULL);
	g_slist_free (repos);

	// don't delete pools as they don't actually belong to Ypp, just listeners
	g_slist_free (pkg_listeners);
	delete disk;
}

const Ypp::Repository *Ypp::Impl::getRepository (int nb)
{
	if (!repos) {
		struct inner {
			static void addRepo (Ypp::Impl *impl, const zypp::RepoInfo &info)
			{
				Repository *repo = new Repository();
				repo->name = info.name();
				if (!info.baseUrlsEmpty())
					repo->url = info.baseUrlsBegin()->asString();
				repo->alias = info.alias();
				repo->enabled = info.enabled();
				impl->repos = g_slist_append (impl->repos, repo);
			}
		};
		for (zypp::ResPoolProxy::repository_iterator it = zyppPool().knownRepositoriesBegin();
		     it != zyppPool().knownRepositoriesEnd(); it++) {
			const zypp::Repository &zrepo = *it;
			if (zrepo.isSystemRepo())
				continue;
			inner::addRepo (this, zrepo.info());
		}
		// zyppPool::knownRepositories is more accurate, but it doesn't feature disabled
		// repositories. Add them with the following API.
		zypp::RepoManager manager;
		std::list <zypp::RepoInfo> known_repos = manager.knownRepositories();
		for (std::list <zypp::RepoInfo>::const_iterator it = known_repos.begin();
		     it != known_repos.end(); it++) {
			const zypp::RepoInfo info = *it;
			if (info.enabled())
				continue;
			inner::addRepo (this, info);
		}
	}
	return (Repository *) g_slist_nth_data (repos, nb);
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
		std::list <std::pair <const Package *, const Repository *> > confirmPkgs;
		for (GSList *p = packages [Ypp::Package::PACKAGE_TYPE]; p; p = p->next) {
			Ypp::Package *pkg = (Ypp::Package *) p->data;
			if (pkg->impl->isTouched()) {
				const Ypp::Package::Version *version = 0;
				if (pkg->toInstall (&version)) {
					if (version->repo != repo) {
						std::pair <const Package *, const Repository *> p (pkg, version->repo);
						confirmPkgs.push_back (p);
					}
				}
			}
		}
		if (!confirmPkgs.empty())
			cancel = (interface->allowRestrictedRepo (confirmPkgs) == false);
	}

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
				for (GSList *p = packages [type]; p; p = p->next) {
					Ypp::Package *pkg = (Ypp::Package *) p->data;
					if (pkg->impl->isTouched()) {
						bool isAuto = pkg->isAuto();
						if ((order == 0 && !isAuto) || (order == 1 && isAuto)) {
							for (GSList *i = pkg_listeners; i; i = i->next)
								((PkgListener *) i->data)->packageModified (pkg);
							pkg->impl->setNotTouched();
						}
					}
				}
			}
	}
	g_slist_free (transactions);
	transactions = NULL;
	inTransaction = false;

	if (disk)
		disk->impl->packageModified();
}

Ypp::Ypp()
{
	impl = new Impl();

    zyppPool().saveState<zypp::Package  >();
    zyppPool().saveState<zypp::Pattern  >();
    zyppPool().saveState<zypp::Patch    >();
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

void Ypp::addPkgListener (PkgListener *listener)
{ impl->pkg_listeners = g_slist_append (impl->pkg_listeners, listener); }
void Ypp::removePkgListener (PkgListener *listener)
{ impl->pkg_listeners = g_slist_remove (impl->pkg_listeners, listener); }

bool Ypp::isModified()
{
	return zyppPool().diffState<zypp::Package  >() ||
	       zyppPool().diffState<zypp::Pattern  >() ||
	       zyppPool().diffState<zypp::Patch    >();
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
//			if (kind == "package")
			if (kind == "pattern")
				type = Ypp::Package::PATTERN_TYPE;

			Ypp::Package *pkg = findPackage (type, name);
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

