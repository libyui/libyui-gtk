/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A simplification of libzypp's API.

   To get a list of packages, setup a Query object and create a Pool with
   it. Package has a set of manipulation methods, the results of which are
   then reported to Pool listeners, which you can choose to act on your
   interface, if you want them reflected on the viewer.
   Iterate the pool using Pool::Iter. Don't keep it around though, instead use
   getIndex() and getIter(index).

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
			PACKAGE_TYPE, PATTERN_TYPE, LANGUAGE_TYPE, TOTAL_TYPES
		};

		Type type();
		const std::string &name();
		const std::string &summary();
		Node *category();
		bool is3D();

		std::string description();
		std::string filelist();
		std::string changelog();
		std::string authors();

		std::string provides();
		std::string requires();

		bool isInstalled();
		bool hasUpgrade();
		bool isLocked();

		bool toInstall (int *nb = 0);
		bool toRemove();
		bool isModified();
		bool isAuto(); /* installing/removing cause of dependency */

		void install (int nb);  /* if installed, will replace it by that version */
		void remove();
		void undo();
		void lock (bool lock);

		struct Version {
			std::string number;
			int repo, cmp /*relatively to installed*/;
			void *impl;
		};
		const Version *getInstalledVersion();
		const Version *getAvailableVersion (int nb);

		// convinience -- true if any available is from the given repo
		bool isOnRepository (int repo);

		struct Impl;
		Impl *impl;
		Package (Impl *impl);
		~Package();
	};

	// when installing/removing/... a few packages at a time, you should use this methods,
	// so that problem resolver gets only kicked after they are all queued
	void startTransactions();
	void finishTransactions();

	// Query
	struct Query {
		Query (Package::Type type);
		void setName (std::string name);
		void setCategory (Ypp::Node *category);
		void setIsInstalled (bool installed);
		void setHasUpgrade (bool upgradable);
		void setIsModified (bool modified);
		void setRepositories (std::list <int> repositories);

		~Query();
		struct Impl;
		Impl *impl;
	};

	// Pool
	struct Pool {
		Pool (Query *query);

		typedef void * Iter;
		Iter getFirst();
		Iter getNext (Iter it);
		Package *get (Iter it);
		int getIndex (Iter it);
		Iter getIter (int index);

		struct Listener {
			virtual void entryChanged  (Iter iter, Package *package) = 0;
			virtual void entryInserted (Iter iter, Package *package) = 0;
			virtual void entryDeleted  (Iter iter, Package *package) = 0;
		};
		void setListener (Listener *listener);

		~Pool();
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
		virtual bool resolveProblems (std::list <Problem *> problems) = 0;

		// FIXME: this is mostly a hack; the thing is that GtkTreeSelection doesn't
		// signal when a selected row changes values. Anyway, to be done differently.
		virtual void packageModified (Package *package) = 0;
	};
	void setInterface (Interface *interface);

	bool isModified();  // any change made?

	// Misc
	struct Repository {
		std::string name, alias /*internal use*/;
	};
	const Repository *getRepository (int nb);

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

	Ypp();
	~Ypp();
	struct Impl;
	Impl *impl;
};

#endif /*ZYPP_WRAPPER_H*/

