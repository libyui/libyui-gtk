/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Simplifies, unifies and extends libzypp's API.

   Several classes are available to wrap around common Zypp objects
   in order to simplify or extend them. These methods can be interwined
   with direct libzypp use, but you should call Ypp::notifySelModified()
   to broadcast any change to a selectable.

   Use Ypp::QueryPool to iterate through the Selectable pool.
   It extends zypp::PoolQuery by adding the possibility to add
   custom criterias for filtering, to be apply endogenously.

   If you need to iterate through it several times, you can create
   a Ypp::List out of it. This random-access list can be manually
   manipulated with several available methods such as for sorting.
   The list is ref-counted so you can easily and freely hold it at
   several widgets.
   In order to inspect several common properties of a group of packages,
   pass a list to Ypp::ListProps.

   Usage example (unlock all packages):

       Ypp::PoolQuery query (Ypp::Selectable::PACKAGE_TYPE);
       query.addCriteria (new Ypp::StatusMatch (Ypp::StatusMatch::IS_LOCKED));

       Ypp::List list (query);
       list.unlock();

   You are advised to register an Ypp::Interface implementation as some
   transactions are bound by user decisions. Call Ypp::init() and
   Ypp::finish() when you begin or are done (respectively) using these
   methods.
   Use Ypp::addSelListener() to be notified of any 'selectable' change.
*/

#ifndef ZYPP_WRAPPER_H
#define ZYPP_WRAPPER_H

#include <zypp/ZYppFactory.h>
#include <zypp/ResObject.h>
#include <zypp/ResKind.h>
#include <zypp/ResPoolProxy.h>
#include <zypp/PoolQuery.h>
#include <zypp/ui/Selectable.h>
#include <zypp/Patch.h>
#include <zypp/Package.h>
#include <zypp/Pattern.h>
#include <zypp/Product.h>
#include <zypp/Repository.h>
#include <zypp/RepoManager.h>
#include <zypp/sat/LocaleSupport.h>
#include <string>
#include <list>
#include "yzypptags.h"

typedef zypp::ResPool             _ZyppPool;
typedef zypp::ResPoolProxy        ZyppPool;
inline ZyppPool zyppPool() { return zypp::getZYpp()->poolProxy(); }
inline _ZyppPool _zyppPool() { return zypp::getZYpp()->pool(); }
typedef zypp::ResObject::constPtr ZyppResObject;
typedef zypp::ResObject*          ZyppResObjectPtr;
typedef zypp::ui::Selectable::Ptr ZyppSelectable;
typedef zypp::ui::Selectable*     ZyppSelectablePtr;
typedef zypp::Package::constPtr   ZyppPackage;
typedef zypp::Patch::constPtr     ZyppPatch;
typedef zypp::Pattern::constPtr   ZyppPattern;
typedef zypp::Repository          ZyppRepository;
typedef zypp::PoolQuery           ZyppQuery;
typedef zypp::sat::SolvAttr       ZyppAttribute;
typedef zypp::ByteCount           Size_t;
typedef zypp::DiskUsageCounter::MountPoint    ZyppDu;
typedef zypp::DiskUsageCounter::MountPointSet ZyppDuSet;

inline ZyppPackage castZyppPackage (ZyppResObject obj)
{ return zypp::dynamic_pointer_cast <const zypp::Package> (obj); }
inline ZyppPatch castZyppPatch (ZyppResObject obj)
{ return zypp::dynamic_pointer_cast <const zypp::Patch> (obj); }
inline ZyppPattern castZyppPattern (ZyppResObject obj)
{ return zypp::dynamic_pointer_cast <const zypp::Pattern> (obj); }

namespace Ypp
{
	struct List;

	// Repositories setup

	struct Repository {
		// merges zypp::Repository and zypp::RepoInfo -- the first are
		// the actual repository structure (which includes only the
		// enabled ones), the others are from the setup file.

		Repository (zypp::Repository repo);
		Repository (zypp::RepoInfo repo);
		std::string name();
		std::string url();
		bool enabled();
		bool isOutdated();
		bool isSystem();

		bool operator == (const Repository &other) const;

		ZyppRepository &zyppRepo() { return m_repo; }

		private:
			ZyppRepository m_repo;
			zypp::RepoInfo m_repo_info;
			bool m_onlyInfo;
	};

	void getRepositoryFromAlias (const std::string &alias,
		std::string &name, std::string &url);

	// Selectable & complementory structs

	struct Version {
		Version (ZyppResObject zobj);

		int type();  // Ypp::Selectable::Type

		std::string number();
		std::string arch();
		Repository repository();

		Size_t size();
		Size_t downloadSize();

		bool isInstalled();
		bool toModify();

		bool operator < (Version &other);
		bool operator > (Version &other);
		bool operator == (Version &other);

		ZyppResObject zyppObj() { return m_zobj; }

		private:
			ZyppResObject m_zobj;
	};

	struct Selectable {
		enum Type {
			PACKAGE, PATTERN, LANGUAGE, PATCH, ALL
		};
		static zypp::ResKind asKind (Type type);
		static Type asType (zypp::ResKind kind);

		Selectable (ZyppSelectable sel);
		Selectable (zypp::Locale locale);

		Type type();
		std::string name();
		std::string summary();
		std::string description (bool as_html);

		bool visible();

		bool isInstalled();
		bool hasUpgrade();
		bool isLocked();

		bool toInstall();
		bool toRemove();
		bool toModify();
		bool toModifyAuto();

		void install();  // installs candidate
		void remove();
		void undo();
		void lock (bool lock);

		bool canRemove();
		bool canLock();

		int totalVersions();
		Version version (int n);

		bool hasCandidateVersion();
		Version candidate();
		void setCandidate (Version &version);

		bool hasInstalledVersion();
		Version installed();
		Version anyVersion();

		bool operator == (const Selectable &other) const;
		bool operator != (const Selectable &other) const;

		ZyppSelectable zyppSel() { return m_sel; }
		zypp::Locale zyppLocale() { return m_locale; }

		private:
			Type m_type;
			ZyppSelectable m_sel;
			zypp::Locale m_locale;
	};

	struct Collection {
		Collection (Selectable &sel);
		bool contains (Selectable &sel);
		void stats (int *installed, int *total);

		Ypp::List *getContent();  // cached

		Selectable &asSelectable() { return m_sel; }

		private:
			Selectable m_sel;
	};

	struct Package {
		Package (Selectable &sel);
		int support();
		static int supportTotal();
		static std::string supportSummary (int support);
		static std::string supportDescription (int support);

		std::string url();

		YPkgGroupEnum group();
		std::string rpm_group();

		bool isCandidatePatch();
		Selectable getCandidatePatch();

		private:
			Selectable m_sel;
	};

	struct Patch {
		Patch (Selectable &sel);
		int priority();
		static int priorityTotal();
		static const char *prioritySummary (int priority);
		static const char *priorityIcon (int priority);

		private:
			Selectable m_sel;
	};

	struct SelListener {
		// a selectable modification has ripple effects; we don't bother
		// detecting them
		virtual void selectableModified() = 0;
	};

	void addSelListener (SelListener *listener);
	void removeSelListener (SelListener *listener);

	// you shouldn't need to call notifySelModified() directly, but instead call
	// e.g. runSolver()
	void notifySelModified();

	struct Problem {
		std::string description, details;
		struct Solution {
			std::string description, details;
			bool apply;
			void *impl;
		};
		Solution *getSolution (int nb);
		void *impl;
	};

	// runSolver() gets called automatically when you install/remove/... a
	// a package using the Ypp::Selectable API
	bool runSolver (bool force = false); // returns whether succesful
	void setEnableSolver (bool enabled);  // true by default
	bool isSolverEnabled();
	bool showPendingLicenses (Ypp::Selectable::Type type);

	// temporarily suspends run-solver while installing/removing a few packages at a time
	// -- used by Ypp::List
	void startTransactions();
	bool finishTransactions();  // returns return of runSolver()

	struct Interface {
		virtual bool showLicense (Selectable &sel, const std::string &license) = 0;
		virtual bool showMessage (Selectable &sel, const std::string &message) = 0;
		// resolveProblems = false to cancel the action that had that effect
		virtual bool resolveProblems (const std::list <Problem *> &problems) = 0;
	};

	void setInterface (Interface *interface);
	Interface *getInterface();

	// Pool selectables

	struct Match {
		virtual bool match (Selectable &sel) = 0;
		virtual ~Match() {}
	};

	struct StrMatch : public Match {
		// exclusive match -- zypp only supports or'ed strings
		// you can combine attributes (inclusive)
		enum Attribute {
			NAME = 0x1, SUMMARY = 0x2, DESCRIPTION = 0x4
		};  // use regular zypp methods for other attributes
		StrMatch (int attrbs);
		void add (const std::string &str);
		virtual bool match (Selectable &sel);

		virtual ~StrMatch();
		struct Impl;
		Impl *impl;
	};

	struct StatusMatch : public Match {
		enum Status {
			IS_INSTALLED, NOT_INSTALLED, HAS_UPGRADE, IS_LOCKED, TO_MODIFY
		};
		StatusMatch (Status status);
		virtual bool match (Selectable &sel);

		private: Status m_status;
	};

	struct PKGroupMatch : public Match {
		PKGroupMatch (YPkgGroupEnum group);
		virtual bool match (Selectable &sel);

		private: YPkgGroupEnum m_group;
	};

	struct RpmGroupMatch : public Match {
		RpmGroupMatch (const std::string &group);
		virtual bool match (Selectable &sel);

		private: std::string m_group;
	};

	struct FromCollectionMatch : public Match {
		FromCollectionMatch (Collection &col);
		virtual bool match (Selectable &sel);

		private: Collection m_collection;
	};

	struct CollectionContainsMatch : public Match {
		CollectionContainsMatch (Selectable &sel);
		virtual bool match (Selectable &collection);

		private: Selectable m_contains;
	};

	struct SupportMatch : public Match {
		SupportMatch (int n) : m_n (n) {}
		virtual bool match (Selectable &sel)
		{ return Package (sel).support() == m_n; }
		private: int m_n;
	};  // we should replace some of these stuff by generic properties

	struct PriorityMatch : public Match {
		PriorityMatch (int n) : m_n (n) {}
		virtual bool match (Selectable &sel)
		{ return Patch (sel).priority() == m_n; }
		private: int m_n;
	};

	struct Query {
		virtual void addCriteria (Match *match) = 0;  // to be free'd by Query
		virtual bool hasNext() = 0;
		virtual Selectable next() = 0;
		virtual int guessSize() = 0;  // only knows for sure after iterating
	};

	struct PoolQuery : public Query {
		PoolQuery (Selectable::Type type);  // for languages, use LangQuery
		~PoolQuery();

		enum MatchType {
			CONTAINS, EXACT, GLOB, REGEX
		};
		enum StringAttribute {
			NAME, SUMMARY, DESCRIPTION, FILELIST, PROVIDES, REQUIRES,
		};
		void setStringMode (bool caseSensitive, MatchType type);
		void addStringAttribute (StringAttribute attrb);
		void addStringOr (const std::string &str);

		void addRepository (Repository &repository);

		virtual void addCriteria (Match *match);  // exclusive

		virtual bool hasNext();  // only after setup
		virtual Selectable next();
		virtual int guessSize();

		ZyppQuery &zyppQuery();
		Selectable::Type poolType();

		struct Impl;
		Impl *impl;
		private:  // prevent copy
			PoolQuery (const PoolQuery&); PoolQuery &operator= (const PoolQuery&);
	};

	struct LangQuery : public Query {
		LangQuery();
		~LangQuery();

		virtual void addCriteria (Match *match) {} /* not yet supported */
		virtual bool hasNext();
		virtual Selectable next();
		virtual int guessSize();

		struct Impl;
		Impl *impl;

		private:  // prevent copy
			LangQuery (const LangQuery&); LangQuery &operator= (const LangQuery&);
	};

	// Aggregators

	struct List {
		List (Query &query);
		List (int reserve);
		~List();
		List clone();

		Selectable &get (int index);
		int size();

		int count (Match *match);
		int find (const std::string &name);
		int find (Selectable &sel);

		void reserve (int size);
		void append (Selectable sel);

		void install();
		void remove();
		void lock (bool lock);
		void undo();

		enum SortAttribute {
			IS_INSTALLED_SORT, NAME_SORT, SIZE_SORT, REPOSITORY_SORT, SUPPORT_SORT
		};
		void sort (SortAttribute attrb, bool ascendent);
		void reverse();

		bool operator == (const Ypp::List &other) const;
		bool operator != (const Ypp::List &other) const;

		struct Impl;
		Impl *impl;
		List (const List&);  // ref-counted
		List &operator= (const List&);
	};

	struct ListProps {
		ListProps (List &list);
		~ListProps();

		bool isInstalled() const;
		bool isNotInstalled() const;
		bool hasUpgrade() const;
		bool toModify() const;
		bool isLocked() const;
		bool isUnlocked() const;
		bool canRemove() const;
		bool canLock() const;

		int isInstalledNb() const;
		int isNotInstalledNb() const;
		int hasUpgradeNb() const;
		int isLockedNb() const;
		int toModifyNb() const;

		struct Impl;
		Impl *impl;
		private:  // prevent copy
			ListProps (const ListProps&); ListProps &operator= (const ListProps&);
	};

	// Disk

	std::vector <std::string> getPartitionList();
	const ZyppDu getPartition (const std::string &mount_point);

	// Misc utilities

	struct Busy {
		// pass 0 as size to only show busy cursor
		Busy (int size);
		~Busy();
		void inc();

		struct Impl;
		Impl *impl;
		private:  // prevent copy
			Busy (const Busy&); Busy &operator= (const Busy&);
	};

	struct BusyListener {
		virtual void loading (float progress) = 0;
	};

	void setBusyListener (BusyListener *listener);

	void init();  // ensures the floor is clean
	void finish();  // ensures the floor is clean
	bool isModified();  // anything changed?
};

#endif /*ZYPP_WRAPPER_H*/

