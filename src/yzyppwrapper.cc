/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Ypp, zypp wrapper */
// check the header file for information about this wrapper

#include <config.h>
#include "YGi18n.h"
#include "yzyppwrapper.h"
#include <string.h>
#include <string>

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

//#define OLD_ZYPP

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

	Ypp::Node *add (const std::string &tree_str)
	{
		const std::list <std::string> nodes_str = YGUtils::splitString (tree_str, delim);
		GNode *parent = root, *sibling = 0;
		Ypp::Node *ret = 0;

		std::list <std::string>::const_iterator it;
		for (it = nodes_str.begin(); it != nodes_str.end(); it++) {
			bool found = false;
			for (sibling = parent->children; sibling; sibling = sibling->next) {
				Ypp::Node *yNode = (Ypp::Node *) sibling->data;
				int cmp = (*compare) (it->c_str(), yNode->name.c_str());
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

		for (; it != nodes_str.end(); it++) {
			Ypp::Node *yNode = new Ypp::Node();
			GNode *n = g_node_new ((void *) yNode);
			yNode->name = *it;
			yNode->impl = (void *) n;
			g_node_insert_before (parent, sibling, n);
			parent = n;
			sibling = NULL;
			ret = yNode;
		}
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
	int getRepository (const std::string &zyppId);
	Disk *getDisk();

	// for Packages
	bool acceptLicense (Ypp::Package *package, const std::string &license);
	void packageModified (Ypp::Package *package);

	// for Pools
	void addPool (Pool::Impl *pool);
	void removePool (Pool::Impl *pool);
	GSList *getPackages (Package::Type type);

private:
	bool resolveProblems();
	Node *addCategory (Ypp::Package::Type type, const std::string &str);
	void polishCategories (Ypp::Package::Type type);

	void startTransactions();
	void finishTransactions();

	friend class Ypp;
	GSList *packages [Package::TOTAL_TYPES];  // primitive pools
	StringTree *categories [Package::TOTAL_TYPES];
	GSList *repos;
	Disk *disk;
	Interface *interface;
	GSList *pools;
	bool inTransaction;
	GSList *transactions;
};

//** Package

struct Ypp::Package::Impl
{
	Impl (Type type, ZyppSelectable sel, Node *category)
	: type (type), zyppSel (sel), category (category),
	  availableVersions (NULL), installedVersion (NULL)
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
	}

	inline bool isModified()
	{ return curStatus != zyppSel->status(); }
	inline void setUnmodified()
	{ curStatus = zyppSel->status(); }

	std::string name, summary;
	Type type;
	ZyppSelectable zyppSel;
	Ypp::Node *category;
	GSList *availableVersions;
	Version *installedVersion;
	bool hasUpgrade;
	zypp::ui::Status curStatus;  // so we know if resolver touched it
};

Ypp::Package::Package (Ypp::Package::Impl *impl)
: impl (impl)
{}
Ypp::Package::~Package()
{ delete impl; }

Ypp::Package::Type Ypp::Package::type()
{ return impl->type; }

const std::string &Ypp::Package::name()
{
	std::string &ret = impl->name;
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
		if (impl->type == PACKAGE_TYPE || impl->type == PATCH_TYPE)
			ret = impl->zyppSel->theObj()->summary();
	}
	return ret;
}

std::string Ypp::Package::description()
{
	ZyppObject object = impl->zyppSel->theObj();
	std::string text = object->description(), br = "<br>";
	// if it has this header, then it is HTML
	const char *header = "<!-- DT:Rich -->", header_len = 16;

	if (!text.compare (0, header_len, header, header_len))
		;
	else {
		YGUtils::escapeMarkup (text);
		YGUtils::replace (text, "\n\n", 2, "<br>");  // break every double line
	}

	if (impl->type == PACKAGE_TYPE) {
		ZyppPackage package = tryCastToZyppPkg (object);
		std::string url = package->url(), license = package->license();
		if (!url.empty())
			text += br + "<b>" + _("Website:") + "</b> " + url;
		if (!license.empty())
			text += br + "<b>" + _("License:") + "</b> " + license;
	}
	else if (impl->type == PATCH_TYPE) {
		ZyppPatch patch = tryCastToZyppPatch (object);
		if (patch->reboot_needed())
			text += br + br + "<b>" + _("Reboot needed!") + "</b>";
	}
	if (impl->type != PATCH_TYPE)
		text += br + "<b>" + _("Size:") + "</b> " + object->size().asString();
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
			tree.add (*it);

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
		// Some packagers put Authors over the Descriptions field. This seems to be rare
		// on, so I'm not even going to bother.

		if (!packager.empty())
			text += _("Packaged by:") + ("<blockquote>" + packager) + "</blockquote>";
		if (!authors.empty())
			text += _("Developed by:") + ("<blockquote>" + authors) + "</blockquote>";
	}
	return text;
}

std::string Ypp::Package::provides()
{
	std::string text;
	ZyppObject object = impl->zyppSel->theObj();
#ifdef OLD_ZYPP
	const zypp::CapSet &capSet = object->dep (zypp::Dep::PROVIDES);
	for (zypp::CapSet::const_iterator it = capSet.begin();
	     it != capSet.end(); it++) {
#else
    const zypp::Capabilities &capSet = object->dep (zypp::Dep::PROVIDES);
    for (zypp::Capabilities::const_iterator it = capSet.begin();
         it != capSet.end(); it++) {
#endif
		if (!text.empty())
			text += "\n";
		text += it->asString();
	}
	return text;
}

std::string Ypp::Package::requires()
{
	std::string text;
	ZyppObject object = impl->zyppSel->theObj();
#ifdef OLD_ZYPP
	const zypp::CapSet &capSet = object->dep (zypp::Dep::REQUIRES);
	for (zypp::CapSet::const_iterator it = capSet.begin();
	     it != capSet.end(); it++) {
#else
    const zypp::Capabilities &capSet = object->dep (zypp::Dep::REQUIRES);
    for (zypp::Capabilities::const_iterator it = capSet.begin();
         it != capSet.end(); it++) {
#endif
		if (!text.empty())
			text += "\n";
		text += it->asString();
	}
	return text;
}

Ypp::Node *Ypp::Package::category()
{
	return impl->category;
}

bool Ypp::Package::fromCollection (Ypp::Package *collection)
{
	switch (collection->type()) {
		case Ypp::Package::PATTERN_TYPE:
		{
			ZyppSelectable selectable = collection->impl->zyppSel;
			ZyppObject object = selectable->theObj();
			ZyppPattern pattern = tryCastToZyppPattern (object);

			const std::set <std::string> &packages = pattern->install_packages();
			for (std::set <std::string>::iterator it = packages.begin();
			     it != packages.end(); it++) {
				if (this->impl->zyppSel->name() == *it)
					return true;
			}
			break;
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
	if (impl->type == Ypp::Package::PATCH_TYPE) {
		if (impl->zyppSel->hasInstalledObj()) {
			// broken? show as available
			if (impl->zyppSel->installedPoolItem().status().isIncomplete())
				return false;
		}
	}

	return impl->zyppSel->hasInstalledObj();
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

bool Ypp::Package::toInstall (int *nb)
{
	if (nb) {
		ZyppObject candidate = impl->zyppSel->candidateObj();
		for (int i = 0; getAvailableVersion (i); i++) {
			const Version *version = getAvailableVersion (i);
			ZyppObject i_obj = (ZyppObjectPtr) version->impl;
			if (i_obj == candidate) {
				*nb = i;
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

void Ypp::Package::install (int nb)
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

	impl->zyppSel->set_status (status);
	if (toInstall()) {
		const Version *version = getAvailableVersion (nb);
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

	impl->zyppSel->set_status (status);
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

	impl->zyppSel->set_status (status);
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

	impl->zyppSel->set_status (status);
	ypp->impl->packageModified (this);
}

static Ypp::Package::Version *constructVersion (ZyppObject object)
{
	Ypp::Package::Version *version = new Ypp::Package::Version();
	version->number = object->edition().version();
	version->number += " (" + object->arch().asString() + ")";
#ifdef OLD_ZYPP
	version->repo = ypp->impl->getRepository (object->repository().info().alias());
#else
	version->repo = ypp->impl->getRepository (object->repoInfo().alias());
#endif
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
	}
	return (Version *) g_slist_nth_data (impl->availableVersions, nb);
}

bool Ypp::Package::isOnRepository (int repo_nb)
{
	int i;
	for (i = 0; getAvailableVersion (i); i++)
		if (getAvailableVersion (i)->repo == repo_nb)
			return true;
	// if there are no availables, answer yes to all repos
	return i == 0;
}

//** Query

struct Ypp::Query::Impl
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
	Keys <Ypp::Node *> categories;
	Keys <Ypp::Package *> collections;
	Keys <int> repositories;
	Key <bool> isInstalled;
	Key <bool> hasUpgrade;
	Key <bool> isModified;

	Impl()
	{}

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
		if (match && names.defined) {
			const std::list <std::string> &values = names.values;
			std::list <std::string>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++) {
				const char *key = it->c_str();
				if (!strcasestr (package->name().c_str(), key) &&
				    !strcasestr (package->summary().c_str(), key) &&
				    !strcasestr (package->provides().c_str(), key)) {
					match = false;
					break;
				}
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
		if (match && repositories.defined) {
			const std::list <int> &values = repositories.values;
			std::list <int>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++)
				if (package->isOnRepository (*it))
					break;
			match = it != values.end();
		}
		if (match && collections.defined) {
			const std::list <Ypp::Package *> &values = collections.values;
			std::list <Ypp::Package *>::const_iterator it;
			for (it = values.begin(); it != values.end(); it++)
				if (package->fromCollection (*it))
					break;
			match = it != values.end();
		}
		return match;
	}
};

Ypp::Query::Query()
{ impl = new Impl(); }
Ypp::Query::~Query()
{ delete impl; }

void Ypp::Query::addType (Ypp::Package::Type value)
{ impl->types.add (value); }
void Ypp::Query::addNames (std::string value, char separator)
{
	if (separator) {
		std::list <std::string> values = YGUtils::splitString (value, separator);
		for (std::list <std::string>::const_iterator it = values.begin();
		     it != values.end(); it++)
			impl->names.add (*it);
	}
	else
		impl->names.add (value);
}
void Ypp::Query::addCategory (Ypp::Node *value)
{ impl->categories.add (value); }
void Ypp::Query::addCollection (Ypp::Package *value)
{ impl->collections.add (value); }
void Ypp::Query::addRepository (int value)
{ impl->repositories.add (value); }
void Ypp::Query::setIsInstalled (bool value)
{ impl->isInstalled.set (value); }
void Ypp::Query::setHasUpgrade (bool value)
{ impl->hasUpgrade.set (value); }
void Ypp::Query::setIsModified (bool value)
{ impl->isModified.set (value); }

//** Pool

struct Ypp::Pool::Impl
{
Query *query;
GSList *packages;
Pool::Listener *listener;

	Impl (Query *query, bool startEmpty)
	: query (query), packages (NULL), listener (NULL)
	{
		if (!startEmpty)
			packages = buildPool (query);
		ypp->impl->addPool (this);
	}

	~Impl()
	{
		ypp->impl->removePool (this);
		delete query;
		g_slist_free (packages);
	}

	void packageModified (Package *package)
	{
		bool match = query->impl->match (package);
		GSList *i;
		for (i = packages; i; i = i->next) {
			if (package == i->data)
				break;
		}
		if (i) {  // is on pool
			if (match) {  // modified
				if (listener)
					listener->entryChanged ((Iter) i, package);
			}
			else {  // removed
				if (listener)
					listener->entryDeleted ((Iter) i, package);
				packages = g_slist_delete_link (packages, i);
			}
		}
		else {  // not on pool
			if (match) {  // inserted
				if (i == NULL) {
					packages = g_slist_append (packages, (gpointer) package);
					i = g_slist_last (packages);
				}
				else {
					packages = g_slist_insert_before (packages, i, (gpointer) package);
					int index = g_slist_position (packages, i) - 1;
					i = g_slist_nth (packages, index);
				}
				if (listener)
					listener->entryInserted ((Iter) i, package);
			}
		}
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

Ypp::Pool::Pool (Query *query, bool startEmpty)
{ impl = new Impl (query, startEmpty); }
Ypp::Pool::~Pool()
{ delete impl; }

Ypp::Pool::Iter Ypp::Pool::getFirst()
{ return impl->packages; }

Ypp::Pool::Iter Ypp::Pool::getNext (Ypp::Pool::Iter iter)
{ return ((GSList *) iter)->next; }

Ypp::Package *Ypp::Pool::get (Ypp::Pool::Iter iter)
{ return (Ypp::Package *) ((GSList *) iter)->data; }

int Ypp::Pool::getIndex (Ypp::Pool::Iter iter)
{ return g_slist_position (impl->packages, (GSList *) iter); }

Ypp::Pool::Iter Ypp::Pool::getIter (int index)
{ return g_slist_nth (impl->packages, index); }

void Ypp::Pool::setListener (Ypp::Pool::Listener *listener)
{ impl->listener = listener; }

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
						zypp::ByteCount (partition->used, zypp::ByteCount::K).asString();
					partition->total_str =
						zypp::ByteCount (partition->total, zypp::ByteCount::K).asString();
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

Ypp::Node *Ypp::getFirstCategory (Ypp::Package::Type type)
{
	if (!impl->getPackages (type))
		return NULL;
	return impl->categories[type]->getFirst();
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
	return categories[type]->add (category_str);
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
: repos (NULL), disk (NULL), interface (NULL), pools (NULL),
  inTransaction (false), transactions (NULL)
{
	for (int i = 0; i < Package::TOTAL_TYPES; i++) {
		packages[i] = NULL;
		categories[i] = NULL;
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

	g_slist_foreach (repos, inner::free_repo, NULL);
	g_slist_free (repos);

	// don't delete pools as they don't actually belong to Ypp, just listeners
	g_slist_free (pools);
	delete disk;
}

const Ypp::Repository *Ypp::Impl::getRepository (int nb)
{
	if (!repos) {
		zypp::RepoManager manager;
		std::list <zypp::RepoInfo> zrepos = manager.knownRepositories();
		for (std::list <zypp::RepoInfo>::const_iterator it = zrepos.begin();
			 it != zrepos.end(); it++) {
			if (it->enabled()) {
				Repository *repo = new Repository();
				repo->name = it->name();
				repo->alias = it->alias();
				repos = g_slist_append (repos, repo);
			}
		}
	}
	return (Repository *) g_slist_nth_data (repos, nb);
}

int Ypp::Impl::getRepository (const std::string &alias)
{
	for (int i = 0; getRepository (i); i++)
		if (getRepository (i)->alias == alias)
			return i;
	return -1; /*error*/
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
	for (GSList *i = pools; i; i = i->next)
		((Pool::Impl *) i->data)->packageModified (package);
	if (interface)
		interface->packageModified (package);

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
	bool resolved = resolveProblems();
	if (resolved) {
		// resolver won't tell us what changed -- tell pools about Auto packages
		for (GSList *p = packages [Ypp::Package::PACKAGE_TYPE]; p; p = p->next) {
			Ypp::Package *pkg = (Ypp::Package *) p->data;
			if (pkg->impl->isModified()) {
				for (GSList *i = pools; i; i = i->next)
					((Pool::Impl *) i->data)->packageModified (pkg);
				pkg->impl->setUnmodified();
			}
		}
	}
	else {
		// user canceled resolver -- undo all
		for (GSList *i = transactions; i; i = i->next)
			((Ypp::Package *) i->data)->undo();
	}
	g_slist_free (transactions);
	transactions = NULL;
	inTransaction = false;

	if (disk)
		disk->impl->packageModified();
}

void Ypp::Impl::addPool (Pool::Impl *pool)
{ pools = g_slist_append (pools, pool); }
void Ypp::Impl::removePool (Pool::Impl *pool)
{ pools = g_slist_remove (pools, pool); }

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
			Ypp::Node *category = 0;
			ZyppObject object = (*it)->theObj();
			// add category and test visibility
			switch (type) {
				case Package::PACKAGE_TYPE:
				{
					ZyppPackage zpackage = tryCastToZyppPkg (object);
					if (!zpackage)
						continue;
					category = addCategory (type, zpackage->group());
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
					if (!(*it)->hasInstalledObj())
						if (!(*it)->hasCandidateObj() || !(*it)->candidatePoolItem().status().isNeeded())
							continue;
					category = addCategory (type, patch->category());
					break;
				}
				default:
					break;
			}

			Package *package = new Package (new Package::Impl (type, *it, category));
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
	delete impl;
}

const Ypp::Repository *Ypp::getRepository (int nb)
{ return impl->getRepository (nb); }

Ypp::Disk *Ypp::getDisk()
{ return impl->getDisk(); }

void Ypp::setInterface (Ypp::Interface *interface)
{ impl->interface = interface; }

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

