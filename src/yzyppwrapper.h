/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A simplification of libzypp's API.

   To get a list of packages, setup a Query object and create a Pool with
   it. Package has a set of manipulation methods, the results of which are
   then reported to Pool listeners, which you can choose to act on your
   interface, if you want them reflected on the viewer.
   Iterate the pool using Pool::Iter. If you want to keep references to them
   around for the UI, you probably want to look at paths.

   You must register an object that implements Interface, as some transactions
   are bound by user decisions. Ypp is a singleton; first call to get() will
   initialize it; you should call finish() when you're done to de-allocate
   caches.
   The only thing you should free is Pool objects. A Query will be freed by
   the Pool object you pass it to.
*/

#ifndef ZYPP_WRAPPER_H
#define ZYPP_WRAPPER_H

#include <string>
#include <list>

struct Ypp
{
	struct Repository;

	// Utilities
	struct Node {
		std::string name;
		Node *next();
		Node *child();
		void *impl;
	};

	// Entries
	struct Package {
		enum Type {
			PACKAGE_TYPE, PATTERN_TYPE, PATCH_TYPE, TOTAL_TYPES
		};

		Type type() const;
		const std::string &name() const;
		const std::string &summary();
		Node *category();
		bool fromCollection (const Ypp::Package *collection) const;

		std::string description();
		std::string filelist();
		std::string changelog();
		std::string authors();

		std::string provides() const;
		std::string requires() const;

		struct Version {
			std::string number;
			const Repository *repo;
			int cmp /* relatively to installed -- ignore if not installed */;
			void *impl;
		};
		const Version *getInstalledVersion();
		const Version *getAvailableVersion (int nb);
		  // convinience -- null if not:
		const Version *fromRepository (const Repository *repo);

		bool isInstalled();
		bool hasUpgrade();
		bool isLocked();

		bool toInstall (const Version **repo = 0);
		bool toRemove();
		bool isModified();
		bool isAuto(); /* installing/removing cause of dependency */

		void install (const Version *repo);  // if installed, will re-install
		                                     // null for most recent version
		void remove();
		void undo();
		void lock (bool lock);

		struct Impl;
		Impl *impl;
		Package (Impl *impl);
		~Package();
	};

	// when installing/removing/... a few packages at a time, you should use this methods,
	// so that problem resolver gets only kicked after they are all queued
	void startTransactions();
	void finishTransactions();

	// Pool
	struct Pool {
		typedef void * Iter;
		virtual Iter getFirst() = 0;
		virtual Iter getNext (Iter it) = 0;
		virtual Iter getParent (Iter it) = 0;
		virtual Iter getChild (Iter it) = 0;
		virtual bool isPlainList() const = 0;

		typedef std::list <int> Path;
		Iter fromPath (const Path &path);
		Path toPath (Iter it);

		virtual std::string getName (Iter it) = 0;
		virtual Package *get (Iter it) = 0; /* may be NULL */
		virtual bool highlight (Iter it) = 0;

		struct Listener {
			virtual void entryInserted (Iter iter, Package *package) = 0;
			virtual void entryDeleted  (Iter iter, Package *package) = 0;
			virtual void entryChanged  (Iter iter, Package *package) = 0;
		};
		void setListener (Listener *listener);

		struct Impl;
		Impl *impl;
		Pool (Impl *impl);
		virtual ~Pool();
	};

	// plain listing of packages as filtered
	// this is a live pool -- entries can be inserted and removed
	struct QueryPool : public Pool {
		struct Query {
			Query();
			void addType (Package::Type type);
			void addNames (std::string name, char separator = 0);
			void addCategory (Ypp::Node *category);
			void addCollection (const Ypp::Package *package);
			void addRepository (const Ypp::Repository *repositories);
			void setIsInstalled (bool installed);
			void setHasUpgrade (bool upgradable);
			void setIsModified (bool modified);

			~Query();
			struct Impl;
			Impl *impl;
		};

		// startEmpty is an optimization when you know the pool won't have
		// elements at creations.
		QueryPool (Query *query, bool startEmpty = false);

		virtual Pool::Iter getFirst();
		virtual Pool::Iter getNext (Pool::Iter it);
		virtual Pool::Iter getParent (Pool::Iter it) { return NULL; }
		virtual Pool::Iter getChild (Pool::Iter it)  { return NULL; }
		virtual bool isPlainList() const { return true; }

		virtual Package *get (Pool::Iter it);
		virtual std::string getName (Pool::Iter it)
		{ return get (it)->name(); }

		virtual bool highlight (Pool::Iter it);

		struct Impl;
		Impl *impl;
	};

	// Pool of categories with the packages as leaves -- only for 1D categories
	// this is a dead pool -- entries can be changed, but not inserted or removed
	struct TreePool : public Pool {
		TreePool (Ypp::Package::Type type);

		virtual Pool::Iter getFirst();
		virtual Pool::Iter getNext (Pool::Iter it);
		virtual Pool::Iter getParent (Pool::Iter it);
		virtual Pool::Iter getChild (Pool::Iter it);
		virtual bool isPlainList() const { return false; }

		virtual std::string getName (Pool::Iter it);
		virtual Package *get (Pool::Iter it); /* may be NULL */
		virtual bool highlight (Pool::Iter it) { return false; }

		struct Impl;
		Impl *impl;
	};

	// Resolver
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

	// Module
	static Ypp *get();
	static void finish();

	struct Interface {
		virtual bool acceptLicense (Package *package, const std::string &license) = 0;
		// resolveProblems = false to cancel the action that had that effect
		virtual bool resolveProblems (const std::list <Problem *> &problems) = 0;
		virtual bool allowRestrictedRepo (const std::list <std::pair <const Package *, const Repository *> > &pkgs) = 0;
	};
	void setInterface (Interface *interface);

	struct PkgListener {
		virtual void packageModified (Package *package) = 0;
	};
	void addPkgListener (PkgListener *listener);
	void removePkgListener (PkgListener *listener);

	// Repositories
	struct Repository {
		std::string name, alias /*internal use*/;
	};
	const Repository *getRepository (int nb);
	void setFavoriteRepository (const Repository *repo);  /* -1 to disable restrictions */
	const Repository *favoriteRepository();

	// Misc
	Package *findPackage (Package::Type type, const std::string &name);

	Node *getFirstCategory (Package::Type type);

	struct Disk
	{
		struct Partition {
			std::string path, used_str, total_str;
			long long used, total;
		};
		const Partition *getPartition (int nb);

		struct Listener {
			virtual void update() = 0;
		};
		void setListener (Listener *listener);

		Disk();
		~Disk();
		struct Impl;
		Impl *impl;
	};
	Disk *getDisk();

	bool isModified();  // any change made?

	bool importList (const char *filename);
	bool exportList (const char *filename);
	bool createSolverTestcase (const char *dirname);

	Ypp();
	~Ypp();
	struct Impl;
	Impl *impl;
};

#endif /*ZYPP_WRAPPER_H*/

