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
		return (Ypp::Node *) root->children->data;
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
	void packageModified (Ypp::Package *package);

	// for the Primitive Pools
	GSList *getPackages (Package::Type type);

private:
	bool resolveProblems();
	Node *addCategory (Ypp::Package::Type type, const std::string &str);
	void polishCategories (Ypp::Package::Type type);
	Node *addCategory2 (Ypp::Package::Type type, const std::string &str);

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
	Impl (Type type, ZyppSelectable sel, Node *category, Node *category2)
	: type (type), zyppSel (sel), category (category), category2 (category2),
	  availableVersions (NULL), installedVersion (NULL), packagesCache (NULL)
	{
		// don't use getAvailableVersion(0) for hasUpgrade() has its inneficient.
		// let's just cache candidate() at start, which should point to the newest version.
		hasUpgrade = false;
		ZyppObject candidate = zyppSel->candidateObj();
		ZyppObject installed = zyppSel->installedObj();
		if (!!candidate && !!installed)
			hasUpgrade = zypp::Edition::compare (candidate->edition(), installed->edition()) > 0;

		setUnmodified();
	}

	~Impl()
	{
		delete installedVersion;
		for (GSList *i = availableVersions; i; i = i->next)
			delete ((Version *) i->data);
		g_slist_free (availableVersions);
		g_slist_free (packagesCache);
	}

	inline bool isModified()
	{ return curStatus != zyppSel->status(); }
	inline void setUnmodified()
	{ curStatus = zyppSel->status(); }

	GSList *getContainedPackages()
	{
		if (!packagesCache) {
			if (type == PATTERN_TYPE) {
				ZyppObject object = zyppSel->theObj();
				ZyppPattern pattern = tryCastToZyppPattern (object);
				zypp::Pattern::Contents contents (pattern->contents());
				for (zypp::Pattern::Contents::Selectable_iterator it =
					contents.selectableBegin(); it != contents.selectableEnd(); it++) {
					packagesCache = g_slist_append (packagesCache, get_pointer (*it));
				}
			}
		}
		return packagesCache;
	}

	std::string name, summary;
	Type type;
	ZyppSelectable zyppSel;
	Ypp::Node *category, *category2;
	GSList *availableVersions;
	Version *installedVersion;
	bool hasUpgrade;
	zypp::ui::Status curStatus;  // so we know if resolver touched it
	// for patterns only:
	GSList *packagesCache;
};

Ypp::Package::Package (Ypp::Package::Impl *impl)
: impl (impl)
{}
Ypp::Package::~Package()
{ delete impl; }

Ypp::Package::Type Ypp::Package::type() const
{ return impl->type; }

const std::string &Ypp::Package::name() const
{
	std::string &ret = const_cast <Package *> (this)->impl->name;
	if (ret.empty()) {
		ZyppSelectable sel = impl->zyppSel;
		ZyppObject obj = sel->theObj();
		switch (impl->type) {
			case PATTERN_TYPE:
				ret = obj->summary();
				break;
#if 0
			case LANGUAGE_TYPE:
				ret = obj->description();
				break;
#endif
			default:
				ret = sel->name();
				break;
		}
	}
	return ret;
}

const std::string &Ypp::Package::summary()
{
	std::string &ret = impl->summary;
	if (ret.empty()) {
		if (impl->type != PATTERN_TYPE)
			ret = impl->zyppSel->theObj()->summary();
	}
	return ret;
}

std::string Ypp::Package::description()
{
	ZyppObject object = impl->zyppSel->theObj();
	std::string text = object->description(), br = "<br>";

	switch (impl->type) {
		case PACKAGE_TYPE:
		{
			// if it has this header, then it is HTML
			const char *header = "<!-- DT:Rich -->", header_len = 16;

			if (!text.compare (0, header_len, header, header_len))
				;
			else {
				// cut authors block
				std::string::size_type i = text.find ("\nAuthors:", 0);
				if (i != std::string::npos) {
					int j = i + sizeof ("\nAuthors:\n");
					if (text.compare (j, sizeof ("-----"), "-----")) {
						text.erase (i);
					}
				}
				while (text.length() > 0 && text [text.length()-1] == '\n')
					text.erase (text.length()-1);

				YGUtils::escapeMarkup (text);
				YGUtils::replace (text, "\n\n", 2, "<br>");  // break every double line
				text += "<br>";
			}

			// specific
			ZyppPackage package = tryCastToZyppPkg (object);
			std::string url = package->url(), license = package->license();
			if (!url.empty())
				text += br + "<b>" + _("Website:") + "</b> <a href=\"" + url + "\">" + url + "</a>";
			if (!license.empty())
				text += br + "<b>" + _("License:") + "</b> " + license;
			text += br + "<b>" + _("Size:") + "</b> " + object->installsize().asString();
			break;
		}
		case PATCH_TYPE:
		{
			ZyppPatch patch = tryCastToZyppPatch (object);
			if (patch->rebootSuggested())
				text += br + br + "<b>" + _("Reboot needed!") + "</b>";
			break;
		}
		case PATTERN_TYPE:
		{
			int installed = 0, total = 0;
			for (GSList *i = impl->getContainedPackages(); i; i = i->next) {
				ZyppSelectablePtr sel = (ZyppSelectablePtr) i->data;
				if (!sel->installedEmpty())
					installed++;
				total++;
			}
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

std::string Ypp::Package::filelist()
{
	std::string text;
	ZyppObject object = impl->zyppSel->installedObj();
	ZyppPackage package = tryCastToZyppPkg (object);
	if (package) {
		StringTree tree (strcmp, '/');

		const std::list <std::string> &filesList = package->filenames();
		for (std::list <std::string>::const_iterator it = filesList.begin();
		     it != filesList.end(); it++)
			tree.add (*it, 0);

		struct inner {
			static std::string getPath (GNode *node)
			{
				Node *yNode  = (Node *) node->data;
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
				Node *yNode  = (Node *) node->data;
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
	return text;
}

std::string Ypp::Package::changelog()
{
	std::string text;
	ZyppObject object = impl->zyppSel->installedObj();
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

std::string Ypp::Package::authors()
{
	std::string text;
	ZyppObject object = impl->zyppSel->theObj();
	ZyppPackage package = tryCastToZyppPkg (object);
	if (package) {
		std::string packager = package->packager(), authors;
		YGUtils::escapeMarkup (packager);
		const std::list <std::string> &authorsList = package->authors();
		for (std::list <std::string>::const_iterator it = authorsList.begin();
		     it != authorsList.end(); it++) {
			std::string author = *it;
			YGUtils::escapeMarkup (author);
			if (!authors.empty())
				authors += "<br>";
			authors += author;
		}
		// look for Authors line in description
		std::string description = package->description();
		std::string::size_type i = description.find ("\nAuthors:", 0);
		if (i != std::string::npos) {
			i += sizeof ("\nAuthors:\n");
			if (description.compare (i, sizeof ("-----"), "-----")) {
				i = description.find ("\n", i+1);
				if (i != std::string::npos) {
					std::string str = description.substr (i+1);
					YGUtils::escapeMarkup (str);
					YGUtils::replace (str, "\n", 1, "<br>");
					authors += str;
					
				}
			}
		}

		if (!authors.empty())
			text += _("Developed by:") + ("<blockquote>" + authors) + "</blockquote>";
		if (!packager.empty())
			text += _("Packaged by:") + ("<blockquote>" + packager) + "</blockquote>";
	}
	return text;
}

std::string Ypp::Package::icon()
{
	if (impl->type == PATTERN_TYPE) {
		ZyppObject object = impl->zyppSel->theObj();
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

bool Ypp::Package::isRecommended() const
{
	return zypp::PoolItem (impl->zyppSel->theObj()).status().isRecommended();
}

bool Ypp::Package::isSuggested() const
{
	return zypp::PoolItem (impl->zyppSel->theObj()).status().isSuggested();
}

std::string Ypp::Package::provides() const
{
	std::string text;
	ZyppObject object = impl->zyppSel->theObj();
    const zypp::Capabilities &capSet = object->dep (zypp::Dep::PROVIDES);
    for (zypp::Capabilities::const_iterator it = capSet.begin();
         it != capSet.end(); it++) {
		if (!text.empty())
			text += "\n";
		text += it->asString();
	}
	return text;
}

std::string Ypp::Package::requires() const
{
	std::string text;
	ZyppObject object = impl->zyppSel->theObj();
    const zypp::Capabilities &capSet = object->dep (zypp::Dep::REQUIRES);
    for (zypp::Capabilities::const_iterator it = capSet.begin();
         it != capSet.end(); it++) {
		if (!text.empty())
			text += "\n";
		text += it->asString();
	}
	return text;
}

Ypp::Node *Ypp::Package::category()
{ return impl->category; }

Ypp::Node *Ypp::Package::category2()
{ return impl->category2; }

bool Ypp::Package::fromCollection (const Ypp::Package *collection) const
{
	switch (collection->type()) {
		case Ypp::Package::PATTERN_TYPE:
		{
			for (GSList *i = collection->impl->getContainedPackages(); i; i = i->next) {
				if (this->impl->zyppSel == i->data)
					return true;
			}
			return false;
		}
#if 0
		case Ypp::Package::LANGUAGE_TYPE:
		{
			ZyppSelectable selectable = collection->impl->zyppSel;
			ZyppObject object = selectable->theObj();
			ZyppLanguage language = tryCastToZyppLanguage (object);

			ZyppObject pkgobj = this->impl->zyppSel->theObj();
			const zypp::Capabilities &capSet = pkgobj->dep (zypp::Dep::FRESHENS);
			for (zypp::Capabilities::const_iterator it = capSet.begin();
			     it != capSet.end(); it++) {
				if (it->index() == language->name())
					return true;
			}
		}
#endif
		default:
			break;
	}
	return false;
}

bool Ypp::Package::isInstalled()
{
	if (!impl->zyppSel->installedEmpty()) {
		if (impl->zyppSel->installedObj().isBroken())
			return false;
		return true;
	}
	return impl->zyppSel->candidateObj().isSatisfied();
}

bool Ypp::Package::hasUpgrade()
{
	return impl->hasUpgrade;
}

bool Ypp::Package::isLocked()
{
	zypp::ui::Status status = impl->zyppSel->status();
	return status == zypp::ui::S_Taboo || status == zypp::ui::S_Protected;
}

bool Ypp::Package::toInstall (const Version **version)
{
	if (version) {
		ZyppObject candidate = impl->zyppSel->candidateObj();
		for (int i = 0; getAvailableVersion (i); i++) {
			const Version *v = getAvailableVersion (i);
			ZyppObject obj = (ZyppObjectPtr) v->impl;
			if (obj == candidate) {
				*version = v;
				break;
			}
		}
	}
	return impl->zyppSel->toInstall();
}

bool Ypp::Package::toRemove()
{
	return impl->zyppSel->toDelete();
}

bool Ypp::Package::isModified()
{
	return impl->zyppSel->toModify();
}

bool Ypp::Package::isAuto()
{
	zypp::ui::Status status = impl->zyppSel->status();
	return status == zypp::ui::S_AutoInstall || status == zypp::ui::S_AutoUpdate ||
	       status == zypp::ui::S_AutoDel;
}

void Ypp::Package::install (const Ypp::Package::Version *version)
{
	if (isLocked())
		return;
	if (!impl->zyppSel->hasLicenceConfirmed())
	{
		const std::string &license = impl->zyppSel->candidateObj()->licenseToConfirm();
		if (!license.empty())
			if (!ypp->impl->acceptLicense (this, license))
				return;
		impl->zyppSel->setLicenceConfirmed();
	}

	zypp::ui::Status status = impl->zyppSel->status();
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

	impl->zyppSel->setStatus (status);
	if (toInstall()) {
		if (!version) {
			version = getAvailableVersion (0);
			const Repository *repo = ypp->favoriteRepository();
			if (repo && fromRepository (repo))
				version = fromRepository (repo);
		}
		ZyppObject candidate = (ZyppObjectPtr) version->impl;
		if (!impl->zyppSel->setCandidate (candidate)) {
			yuiWarning () << "Error: Could not set package '" << name() << "' candidate to '" << version->number << "'\n";
			return;
		}
	}

	ypp->impl->packageModified (this);
}

void Ypp::Package::remove()
{
	zypp::ui::Status status = impl->zyppSel->status();
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

	impl->zyppSel->setStatus (status);
	ypp->impl->packageModified (this);
}

void Ypp::Package::undo()
{
	zypp::ui::Status status = impl->zyppSel->status();
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

	impl->zyppSel->setStatus (status);
	ypp->impl->packageModified (this);
}

void Ypp::Package::lock (bool lock)
{
	undo();

	zypp::ui::Status status;
	if (lock)
		status = isInstalled() ? zypp::ui::S_Protected : zypp::ui::S_Taboo;
	else
		status = isInstalled() ? zypp::ui::S_KeepInstalled : zypp::ui::S_NoInst;

	impl->zyppSel->setStatus (status);
	ypp->impl->packageModified (this);
}

static Ypp::Package::Version *constructVersion (ZyppObject object)
{
	Ypp::Package::Version *version = new Ypp::Package::Version();
	version->number = object->edition().asString();
	version->arch = object->arch().asString();
	version->repo = ypp->impl->getRepository (object->repoInfo().alias());
	version->cmp = 0;
	version->impl = (void *) get_pointer (object);
	return version;
}

const Ypp::Package::Version *Ypp::Package::getInstalledVersion()
{
	if (!impl->installedVersion) {
		const ZyppObject installedObj = impl->zyppSel->installedObj();
		if (installedObj)
			impl->installedVersion = constructVersion (installedObj);
	}
	return impl->installedVersion;
}

const Ypp::Package::Version *Ypp::Package::getAvailableVersion (int nb)
{
	if (!impl->availableVersions) {
#if 0  // for debug purposes, when not online
		Ypp::Package::Version *version;

		version = new Ypp::Package::Version();
		version->number = "5.0.6";
		version->arch = "i586";
		version->repo = 0;
		version->cmp = 1;
		version->impl = NULL;
		impl->availableVersions = g_slist_append (impl->availableVersions, version);

		version = new Ypp::Package::Version();
		version->number = "4.1.2";
		version->arch = "i586";
		version->repo = 1;
		version->cmp = -1;
		version->impl = NULL;
		impl->availableVersions = g_slist_append (impl->availableVersions, version);
#else
		const ZyppObject installedObj = impl->zyppSel->installedObj();
		for (zypp::ui::Selectable::available_iterator it = impl->zyppSel->availableBegin();
			 it != impl->zyppSel->availableEnd(); it++) {
			Version *version = constructVersion (*it);
			if (installedObj)
				version->cmp = zypp::Edition::compare ((*it)->edition(), installedObj->edition());
			impl->availableVersions = g_slist_append (impl->availableVersions, version);
		}
		struct inner {
			static gint version_compare (gconstpointer pa, gconstpointer pb)
			{
				ZyppObjectPtr a = (ZyppObjectPtr) ((Version *) pa)->impl;
				ZyppObjectPtr b = (ZyppObjectPtr) ((Version *) pb)->impl;
				// swapped arguments, as we want them sorted from newer to older
				return zypp::Edition::compare (b->edition(), a->edition());
			}
		};
		impl->availableVersions = g_slist_sort (impl->availableVersions,
		                                        inner::version_compare);
#endif
	}
	return (Version *) g_slist_nth_data (impl->availableVersions, nb);
}

const Ypp::Package::Version *Ypp::Package::fromRepository (const Ypp::Repository *repo)
{
	for (int i = 0; getAvailableVersion (i); i++)
		if (getAvailableVersion (i)->repo == repo)
			return getAvailableVersion (i);
	return NULL;
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
	bool use_name, use_summary, use_description;
	Keys <Node *> categories, categories2;
	Keys <const Package *> collections;
	Keys <const Repository *> repositories;
	Key <bool> isInstalled;
	Key <bool> hasUpgrade;
	Key <bool> isModified;
	Key <bool> isRecommended;
	Key <bool> isSuggested;
	Ypp::Package *highlight;

	Impl()
	{
		highlight = NULL;
	}

	bool match (Package *package)
	{
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
		if (match && isModified.defined)
			match = isModified.is (package->isModified());
		if (match && isRecommended.defined)
			match = isRecommended.is (package->isRecommended());
		if (match && isSuggested.defined)
			match = isSuggested.is (package->isSuggested());
		if (match && names.defined) {
			const std::list <std::string> &values = names.values;
			std::list <std::string>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++) {
				const char *key = it->c_str();
				bool str_match = false;
				if (use_name)
					str_match = strcasestr (package->name().c_str(), key);
				if (!str_match && use_summary)
					str_match = strcasestr (package->summary().c_str(), key);
				if (!str_match && use_description)
					str_match = strcasestr (package->description().c_str(), key);
				if (!str_match) {
					match = false;
					break;
				}
			}
			if (match && !highlight) {
				if (values.size() == 1 && values.front() == package->name())
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
                                       bool use_summary, bool use_description)
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
void Ypp::QueryPool::Query::setIsModified (bool value)
{ impl->isModified.set (value); }
void Ypp::QueryPool::Query::setIsRecommended (bool value)
{ impl->isRecommended.set (value); }
void Ypp::QueryPool::Query::setIsSuggested (bool value)
{ impl->isSuggested.set (value); }

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
		for (Ypp::Node *i = ypp->getFirstCategory (type); i; i = i->next()) {
			GNode *cat = g_node_append_data (root, i);
			for (GSList *p = packages; p; p = p->next) {
				Ypp::Package *pkg = (Ypp::Package *) p->data;
				if (pkg->category() == i) {
					g_node_append_data (cat, pkg);
				}
			}
		}
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
				if (!it->readonly) {
					// partition fields: dir, used_size, total_size (size on Kb)
					Ypp::Disk::Partition *partition = new Partition();
					partition->path = it->dir;
					partition->used = it->pkg_size;
					partition->total = it->total_size;
					partition->used_str =
						zypp::ByteCount (partition->used, zypp::ByteCount::K).asString() + "B";
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
	if (!impl->getPackages (type))
		return NULL;
	return impl->categories[type]->getFirst();
}

Ypp::Node *Ypp::getFirstCategory2 (Ypp::Package::Type type)
{
	if (!impl->getPackages (type))
		return NULL;
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

	if (!categories[type])
		categories[type] = new StringTree (inner::cmp, '/');
	return categories[type]->add (category_str, 0);
}

Ypp::Node *Ypp::Impl::addCategory2 (Ypp::Package::Type type, const std::string &category_str)
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

	YPkgGroupEnum group = zypp_tag_convert (category_str);
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
			Ypp::Node *ynode = pkg->category();
			if (ynode->child()) {
				GNode *node = (GNode *) ynode->impl;
				GNode *last = g_node_last_child (node);
				if (((Ypp::Node *) last->data)->name == "Other")
					pkg->impl->category = (Ypp::Node *) last->data;
				else {
					// must create a "Other" node
					Ypp::Node *yN = new Ypp::Node();
					GNode *n = g_node_new ((void *) yN);
					yN->name = "Other";
					yN->icon = NULL;
					yN->impl = (void *) n;
					g_node_insert_before (node, NULL, n);
					pkg->impl->category = yN;
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
		zypp::RepoManager manager;
		std::list <zypp::RepoInfo> zrepos = manager.knownRepositories();
		for (std::list <zypp::RepoInfo>::const_iterator it = zrepos.begin();
			 it != zrepos.end(); it++) {
			Repository *repo = new Repository();
			repo->name = it->name();
			if (!it->baseUrlsEmpty())
				repo->url = it->baseUrlsBegin()->asString();
			repo->alias = it->alias();
			repo->enabled = it->enabled();
			repos = g_slist_append (repos, repo);
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
	return false;
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
	// notify listeners of package change
	for (GSList *i = pkg_listeners; i; i = i->next)
		((Pool::Impl *) i->data)->packageModified (package);

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
			if (pkg->impl->isModified()) {
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
		for (GSList *p = packages [Ypp::Package::PACKAGE_TYPE]; p; p = p->next) {
			Ypp::Package *pkg = (Ypp::Package *) p->data;
			if (pkg->impl->isModified()) {
				for (GSList *i = pkg_listeners; i; i = i->next)
					((PkgListener *) i->data)->packageModified (pkg);
				pkg->impl->setUnmodified();
			}
		}
	}
	g_slist_free (transactions);
	transactions = NULL;
	inTransaction = false;

	if (disk)
		disk->impl->packageModified();
}

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
#if 0
			case Package::LANGUAGE_TYPE:
				it = zyppPool().byKindBegin <zypp::Language>();
				end = zyppPool().byKindEnd <zypp::Language>();
				break;
#endif
			case Package::PATCH_TYPE:
				it = zyppPool().byKindBegin <zypp::Patch>();
				end = zyppPool().byKindEnd <zypp::Patch>();
				break;
			default:
				break;
		}
		for (; it != end; it++) {
			Ypp::Node *category = 0, *category2 = 0;
			ZyppObject object = (*it)->theObj();
			// add category and test visibility
			switch (type) {
				case Package::PACKAGE_TYPE:
				{
					ZyppPackage zpackage = tryCastToZyppPkg (object);
					if (!zpackage)
						continue;
					category = addCategory (type, zpackage->group());
					category2 = addCategory2 (type, zpackage->group());
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
					category = addCategory (type, patch->category());
					break;
				}
				default:
					break;
			}

			Package *package = new Package (new Package::Impl (type, *it, category, category2));
			pool = g_slist_prepend (pool, package);
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

Ypp::Ypp()
{
	impl = new Impl();

    zyppPool().saveState<zypp::Package  >();
    zyppPool().saveState<zypp::Pattern  >();
   // zyppPool().saveState<zypp::Language >();
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
	     //  zyppPool().diffState<zypp::Language >() ||
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

