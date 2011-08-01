/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* Ypp, the zypp wrapper */
// check the header file for information about this wrapper

/*
  Textdomain "gtk"
 */

#include "config.h"
#include "YGi18n.h"
#define YUILogComponent "gtk-pkg"
#include <YUILog.h>
#include "yzyppwrapper.h"
#include "YGUtils.h"
#include <algorithm>

static Ypp::Interface *g_interface = 0;
static bool g_autoSolver = true;

// Repository

Ypp::Repository::Repository (zypp::Repository repo)
: m_repo (repo), m_repo_info (repo.info()), m_onlyInfo (false) {}

Ypp::Repository::Repository (zypp::RepoInfo repo)
: m_repo (NULL), m_repo_info (repo), m_onlyInfo (true) {}

std::string Ypp::Repository::name()
{ return m_repo_info.name(); }

std::string Ypp::Repository::url()
{ return m_repo_info.url().asString(); }

bool Ypp::Repository::enabled()
{ return m_repo_info.enabled(); }

bool Ypp::Repository::isOutdated()
{ return m_onlyInfo ? false : m_repo.maybeOutdated(); }

bool Ypp::Repository::isSystem()
{ return m_repo.isSystemRepo(); }

bool Ypp::Repository::operator == (const Ypp::Repository &other) const
{ return this->m_repo.info().alias() == other.m_repo.info().alias(); }

void Ypp::getRepositoryFromAlias (const std::string &alias, std::string &name, std::string &url)
{
	static std::map <std::string, zypp::RepoInfo> repos;
	if (repos.empty()) {
		zypp::RepoManager manager;
		std::list <zypp::RepoInfo> known_repos = manager.knownRepositories();
		for (std::list <zypp::RepoInfo>::const_iterator it = known_repos.begin();
			 it != known_repos.end(); it++)
			repos[it->alias()] = *it;
	}

	std::map <std::string, zypp::RepoInfo>::iterator it = repos.find (alias);
	if (it != repos.end()) {
		zypp::RepoInfo *repo = &it->second;
		name = repo->name();
		url = repo->url().asString();
	}
	else
		name = alias;  // return alias if repo not currently setup-ed
}

// Version

Ypp::Version::Version (ZyppResObject zobj)
: m_zobj (zobj) {}

int Ypp::Version::type()
{ return Selectable::asType (m_zobj->kind()); }

std::string Ypp::Version::number()
{ return m_zobj->edition().asString(); }

std::string Ypp::Version::arch()
{ return m_zobj->arch().asString(); }

Ypp::Repository Ypp::Version::repository()
{ return Repository (m_zobj->repository()); }

Size_t Ypp::Version::size()
{ return m_zobj->installSize(); }

Size_t Ypp::Version::downloadSize()
{ return m_zobj->downloadSize(); }

bool Ypp::Version::isInstalled()
{
	zypp::ResStatus status = m_zobj->poolItem().status();
	switch (type()) {
		case Ypp::Selectable::PATCH:
		case Ypp::Selectable::PATTERN:
			return status.isSatisfied() && !status.isToBeInstalled();
		default:
			return status.isInstalled();
	}
}

bool Ypp::Version::toModify()
{
	zypp::ResStatus status = m_zobj->poolItem().status();
	return status.transacts();
}

bool Ypp::Version::operator < (Ypp::Version &other)
{ return this->m_zobj->edition() < other.m_zobj->edition(); }

bool Ypp::Version::operator > (Ypp::Version &other)
{ return this->m_zobj->edition() > other.m_zobj->edition(); }

bool Ypp::Version::operator == (Ypp::Version &other)
{ return this->m_zobj->edition() == other.m_zobj->edition(); }

// Selectable

zypp::ResKind Ypp::Selectable::asKind (Type type)
{
	switch (type) {
		case PATTERN: return zypp::ResKind::pattern;
		case PATCH: return zypp::ResKind::patch;
		case PACKAGE: case LANGUAGE: case ALL: break;
	}
	return zypp::ResKind::package;
}

Ypp::Selectable::Type Ypp::Selectable::asType (zypp::ResKind kind)
{
	if (kind == zypp::ResKind::patch)
		return PATCH;
	if (kind == zypp::ResKind::pattern)
		return PATTERN;
	return PACKAGE;
}

Ypp::Selectable::Selectable (ZyppSelectable sel)
: m_type (asType (sel->kind())), m_sel (sel)
{}

Ypp::Selectable::Selectable (zypp::Locale locale)
: m_type (LANGUAGE), m_locale (locale) {}

Ypp::Selectable::Type Ypp::Selectable::type()
{ return m_type; }

std::string Ypp::Selectable::name()
{
	switch (m_type) {
		case LANGUAGE: return m_locale.name() + " (" + m_locale.code() + ")";
		case PATTERN: return m_sel->theObj()->summary();
		default: break;
	}
	return m_sel->name();
}

std::string Ypp::Selectable::summary()
{
	switch (m_type) {
		case PATTERN:
		case LANGUAGE: {
			Collection col (*this);
			int installed, total;
			col.stats (&installed, &total);
			std::ostringstream stream;
			stream << _("Installed:") << " " << installed << " " << _("of") << " " << total;
			return stream.str();
		}
		default: break;
	}
	return m_sel->theObj()->summary();
}

std::string Ypp::Selectable::description (bool as_html)
{
	if (m_type == LANGUAGE)
		return summary();

	std::string text (m_sel->theObj()->description()), br ("\n");
	if (as_html) br = "<br />";
	switch (m_type) {
		case PACKAGE: {
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
				if (as_html)  // break every double line
					YGUtils::replace (text, "\n\n", 2, "<br>");
				text += br;
			}
			break;
		}
		case PATCH: {
			ZyppPatch patch = castZyppPatch (m_sel->theObj());
			if (patch->rebootSuggested()) {
				text += br + br + "<b>" + _("Reboot required:") + " </b>";
				text += _("the system will have to be restarted in order for "
					"this patch to take effect.");
			}
			if (patch->reloginSuggested()) {
				text += br + br + "<b>" + _("Relogin required:") + " </b>";
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
		case PATTERN: {
			text += br + br + summary();
			break;
		}
		default: break;
	}
	return text;
}

bool Ypp::Selectable::visible()
{
	switch (m_type) {
		case PATTERN: {
			ZyppPattern pattern = castZyppPattern (zyppSel()->theObj());
			return pattern->userVisible();
		}
		case PATCH:
			if (zyppSel()->hasCandidateObj())
				if (!zyppSel()->candidateObj().isRelevant())
					return false;
			return true;
		default: break;
	}
	return true;
}

bool Ypp::Selectable::isInstalled()
{
	switch (m_type) {
		case PACKAGE:
			return !m_sel->installedEmpty();
		case PATTERN:
		case PATCH:
			if (hasCandidateVersion())
				return candidate().isInstalled();
			return true;
		case LANGUAGE:
			return _zyppPool().isRequestedLocale (m_locale);
		case ALL: break;
	}
	return 0;
}

bool Ypp::Selectable::hasUpgrade()
{
	switch (m_type) {
		case LANGUAGE: break;
		default: {
			const ZyppResObject candidate = m_sel->candidateObj();
			const ZyppResObject installed = m_sel->installedObj();
			if (!!candidate && !!installed)
				return zypp::Edition::compare (candidate->edition(), installed->edition()) > 0;
		}
	}
	return false;
}

bool Ypp::Selectable::isLocked()
{
	switch (m_type) {
		case LANGUAGE: break;
		default: {
			zypp::ui::Status status = m_sel->status();
			return status == zypp::ui::S_Taboo || status == zypp::ui::S_Protected;
		}
	}
	return false;
}

bool Ypp::Selectable::toInstall()
{
	switch (m_type) {
		case LANGUAGE: break;
		default: return m_sel->toInstall();
	}
	return false;
}

bool Ypp::Selectable::toRemove()
{
	switch (m_type) {
		case LANGUAGE: break;
		default: return m_sel->toDelete();
	}
	return false;
}

bool Ypp::Selectable::toModify()
{
	switch (m_type) {
		case LANGUAGE: break;
		default: return m_sel->toModify();
	}
	return false;
}

bool Ypp::Selectable::toModifyAuto()
{ 
	switch (m_type) {
		case LANGUAGE: break;
		default:
			return m_sel->toModify() && m_sel->modifiedBy() != zypp::ResStatus::USER;
	}
	return false;
}

void Ypp::Selectable::install()
{
	if (isLocked())
		return;
	switch (m_type) {
		case LANGUAGE:
			if (!_zyppPool().isRequestedLocale (m_locale))
				_zyppPool().addRequestedLocale (m_locale);
			break;
		default: {
			if (!m_sel->hasLicenceConfirmed()) {
				ZyppResObject obj = m_sel->candidateObj();
				if (obj && g_interface && g_autoSolver) {
					const std::string &license = obj->licenseToConfirm();
					if (!license.empty())
						if (!g_interface->showLicense (*this, license))
							return;
					const std::string &msg = obj->insnotify();
					if (!msg.empty())
						if (!g_interface->showMessage (*this, msg))
							return;
				}
				m_sel->setLicenceConfirmed();
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
					if (hasInstalledVersion())
						status = zypp::ui::S_Update;
					else
						status = zypp::ui::S_Install;
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
			break;
		}
	}

	if (!runSolver()) undo();
}

void Ypp::Selectable::remove()
{
	switch (m_type) {
		case LANGUAGE:
			if (_zyppPool().isRequestedLocale (m_locale))
				_zyppPool().eraseRequestedLocale (m_locale);
			break;
		default: {
			if (m_sel->hasCandidateObj() && g_interface && g_autoSolver) {
				ZyppResObject obj = m_sel->candidateObj();
				const std::string &msg = obj->delnotify();
				if (!msg.empty())
					if (!g_interface->showMessage (*this, msg))
						return;
			}

			zypp::ui::Status status = m_sel->status();
			switch (status) {
				// not applicable
				case zypp::ui::S_Protected:
				case zypp::ui::S_Taboo:
				case zypp::ui::S_NoInst:
				case zypp::ui::S_Del:
					break;
				// nothing to do about it
				case zypp::ui::S_AutoInstall:
				case zypp::ui::S_AutoUpdate:
					break;
				// undo
				case zypp::ui::S_Install:
				case zypp::ui::S_Update:
				// action
				case zypp::ui::S_KeepInstalled:
				case zypp::ui::S_AutoDel:
					status = zypp::ui::S_Del;
					break;
			}
			m_sel->setStatus (status);
			break;
		}
	}

	if (!runSolver()) undo();
}

void Ypp::Selectable::undo()
{
	// undo not applicable to language type

	zypp::ui::Status status = m_sel->status();
	zypp::ui::Status prev_status = status;
	switch (status) {
		// not applicable
		case zypp::ui::S_Protected:
		case zypp::ui::S_Taboo:
		case zypp::ui::S_NoInst:
		case zypp::ui::S_KeepInstalled:
			break;
		// undo
		case zypp::ui::S_Install:
		case zypp::ui::S_AutoInstall:
			status = zypp::ui::S_NoInst;
			break;
		case zypp::ui::S_AutoUpdate:
		case zypp::ui::S_AutoDel:
		case zypp::ui::S_Update:
		case zypp::ui::S_Del:
			status = zypp::ui::S_KeepInstalled;
			break;
	}
	m_sel->setStatus (status);

	if (!runSolver()) {
		m_sel->setStatus (prev_status);
		runSolver();
	}
}

void Ypp::Selectable::lock (bool lock)
{
	undo();
	zypp::ui::Status status;
	if (lock)
		status = isInstalled() ? zypp::ui::S_Protected : zypp::ui::S_Taboo;
	else
		status = isInstalled() ? zypp::ui::S_KeepInstalled : zypp::ui::S_NoInst;
	m_sel->setStatus (status);

	if (!runSolver()) undo();
}

bool Ypp::Selectable::canRemove()
{
	switch (m_type) {
		case PACKAGE: case LANGUAGE: return true;
		default: break;
	}
	return false;
}

bool Ypp::Selectable::canLock()
{
	switch (m_type) {
		case PACKAGE:
		//case PATCH:
			return true;
		default: break;
	}
	return false;
}

int Ypp::Selectable::totalVersions()
{ return m_sel->installedSize() + m_sel->availableSize(); }

Ypp::Version Ypp::Selectable::version (int n)
{
	if (n < (signed) m_sel->installedSize()) {
		zypp::ui::Selectable::installed_iterator it = m_sel->installedBegin();
		for (int i = 0; i < n; i++) it++;
		return Version (*it);
	}
	else {
		n -= m_sel->installedSize();
		zypp::ui::Selectable::available_iterator it = m_sel->availableBegin();
		for (int i = 0; i < n; i++) it++;
		return Version (*it);
	}
}

bool Ypp::Selectable::hasCandidateVersion()
{ return !m_sel->availableEmpty(); }

Ypp::Version Ypp::Selectable::candidate()
{ return Version (m_sel->candidateObj()); }

void Ypp::Selectable::setCandidate (Ypp::Version &version)
{
	m_sel->setCandidate (version.zyppObj());
	runSolver();
}

bool Ypp::Selectable::hasInstalledVersion()
{ return !m_sel->installedEmpty(); }

Ypp::Version Ypp::Selectable::installed()
{
	if (m_type == PATCH || m_type == PATTERN) {
		if (m_sel->candidateObj() && m_sel->candidateObj().isSatisfied())
			return Ypp::Version (m_sel->candidateObj());
	}
	return Ypp::Version (m_sel->installedObj());
}

Ypp::Version Ypp::Selectable::anyVersion()
{ return Version (m_sel->theObj()); }

bool Ypp::Selectable::operator == (const Ypp::Selectable &other) const
{ return this->m_sel == other.m_sel; }

bool Ypp::Selectable::operator != (const Ypp::Selectable &other) const
{ return this->m_sel != other.m_sel; }

// Collection

Ypp::Collection::Collection (Ypp::Selectable &sel)
: m_sel (sel) {}

static std::map <ZyppPattern, Ypp::List> g_patternsContent;
static std::map <zypp::Locale, Ypp::List> g_languagesContent;

Ypp::List *Ypp::Collection::getContent()
{  // zypp::Pattern::Contents is expensive to iterate; this makes it cheap
	if (m_sel.type() == Ypp::Selectable::PATTERN) {
		ZyppPattern pattern = castZyppPattern (m_sel.zyppSel()->theObj());
		std::map <ZyppPattern, Ypp::List>::iterator it;
		if ((it = g_patternsContent.find (pattern)) != g_patternsContent.end())
			return &it->second;

		zypp::Pattern::Contents c = pattern->contents();
		Ypp::List list (c.size());
		for (zypp::Pattern::Contents::Selectable_iterator it = c.selectableBegin();
		     it != c.selectableEnd(); it++)
			list.append (Selectable (*it));
		g_patternsContent.insert (std::make_pair (pattern, list));
		return &g_patternsContent.find (pattern)->second;
	}
	else {  // Language
		zypp::Locale locale = m_sel.zyppLocale();
		std::map <zypp::Locale, Ypp::List>::iterator it;
		if ((it = g_languagesContent.find (locale)) != g_languagesContent.end())
			return &it->second;

		zypp::sat::LocaleSupport locale_sup (locale);
		Ypp::List list (50);
		for_( it, locale_sup.selectableBegin(), locale_sup.selectableEnd() ) {
			list.append (Selectable (*it));
		}
		g_languagesContent.insert (std::make_pair (locale, list));
		return &g_languagesContent.find (locale)->second;
	}
}

bool Ypp::Collection::contains (Ypp::Selectable &sel)
{
	if (m_sel.type() == Ypp::Selectable::PATCH) {
		if (m_sel.hasCandidateVersion()) {
			ZyppResObject object = m_sel.zyppSel()->candidateObj();
			ZyppPatch patch = castZyppPatch (object);
			zypp::Patch::Contents contents (patch->contents());
			ZyppSelectable pkg = sel.zyppSel();
			for (zypp::Patch::Contents::Selectable_iterator it =
				 contents.selectableBegin(); it != contents.selectableEnd(); it++) {
				if (*it == pkg)
					return true;
			}
		}
		return false;
	}

	//else
	Ypp::List *content = getContent();
	return content->find (sel) != -1;
}

void Ypp::Collection::stats (int *installed, int *total)
{
	Ypp::List *content = getContent();
	Ypp::ListProps props (*content);
	*installed = props.isInstalledNb();
	*total = content->size();
}

// Package

Ypp::Package::Package (Ypp::Selectable &sel)
: m_sel (sel) {}

int Ypp::Package::support()
{
	ZyppPackage pkg = castZyppPackage (m_sel.zyppSel()->theObj());
	switch (pkg->vendorSupport()) {
		case zypp::VendorSupportUnknown: return 0;
		case zypp::VendorSupportUnsupported: return 1;
		case zypp::VendorSupportACC: return 2;
		case zypp::VendorSupportLevel1: return 3;
		case zypp::VendorSupportLevel2: return 4;
		case zypp::VendorSupportLevel3: return 5;
	}
	return 0;
}

int Ypp::Package::supportTotal()
{ return 6; }

static zypp::VendorSupportOption asSupportOpt (int support)
{
	switch (support) {
		case 0: return zypp::VendorSupportUnknown;
		case 1: return zypp::VendorSupportUnsupported;
		case 2: return zypp::VendorSupportACC;
		case 3: return zypp::VendorSupportLevel1;
		case 4: return zypp::VendorSupportLevel2;
		case 5: return zypp::VendorSupportLevel3;
	}
	return zypp::VendorSupportUnknown;
}

std::string Ypp::Package::supportSummary (int support)
{ return zypp::asUserString (asSupportOpt (support)); }

std::string Ypp::Package::supportDescription (int support)
{ return zypp::asUserStringDescription (asSupportOpt (support)); }

std::string Ypp::Package::url()
{
	ZyppPackage package = castZyppPackage (m_sel.zyppSel()->theObj());
	return package->url();
}

YPkgGroupEnum Ypp::Package::group()
{
	static std::map <ZyppPackage, YPkgGroupEnum> pkgGroupMap;
	ZyppPackage pkg = castZyppPackage (m_sel.zyppSel()->theObj());
	std::map <ZyppPackage, YPkgGroupEnum>::iterator it = pkgGroupMap.find (pkg);
	if (it == pkgGroupMap.end()) {
		YPkgGroupEnum group = zypp_tag_convert (pkg->group());
		pkgGroupMap.insert (std::make_pair (pkg, group));
		return group;
	}
	return it->second;
}

std::string Ypp::Package::rpm_group()
{
	ZyppPackage pkg = castZyppPackage (m_sel.zyppSel()->theObj());
	return pkg->group();
}

static std::map <std::string, ZyppSelectable> g_selPatch;

static ZyppSelectable getSelPatch (Ypp::Selectable &sel)
{
	std::string name (sel.name());
	std::map <std::string, ZyppSelectable>::iterator it = g_selPatch.find (name);
	if (it != g_selPatch.end())
		return it->second;

	Ypp::PoolQuery query (Ypp::Selectable::PATCH);
	query.addCriteria (new Ypp::StatusMatch (Ypp::StatusMatch::NOT_INSTALLED));
	query.addCriteria (new Ypp::CollectionContainsMatch (sel));
	if (query.hasNext()) {
		Ypp::Selectable patch = query.next();
		g_selPatch[name] = patch.zyppSel();
	}
	else
		g_selPatch[name] = NULL;
	return g_selPatch[name];
}

bool Ypp::Package::isCandidatePatch()
{
	if (m_sel.hasCandidateVersion() && m_sel.hasInstalledVersion()) {
		Ypp::Version candidate (m_sel.candidate()), installed (m_sel.installed());
		if (candidate > installed)
			return getSelPatch (m_sel) != NULL;
	}
	return false;
}

Ypp::Selectable Ypp::Package::getCandidatePatch()
{ return Selectable (getSelPatch (m_sel)); }

// Patch

Ypp::Patch::Patch (Ypp::Selectable &sel)
: m_sel (sel) {}

int Ypp::Patch::priority()
{
	ZyppPatch patch = castZyppPatch (m_sel.zyppSel()->theObj());
	switch (patch->categoryEnum()) {
		case zypp::Patch::CAT_SECURITY:    return 0;
		case zypp::Patch::CAT_RECOMMENDED: return 1;
		case zypp::Patch::CAT_YAST:        return 2;
		case zypp::Patch::CAT_DOCUMENT:    return 3;
		case zypp::Patch::CAT_OPTIONAL:    return 4;
		case zypp::Patch::CAT_OTHER:       return 5;
	}
	return 0;
}

int Ypp::Patch::priorityTotal()
{ return 6; }

const char *Ypp::Patch::prioritySummary (int priority)
{
	switch (priority) {
		// Translators: this refers to patch priority
		case 0: return _("Security");
		// Translators: this refers to patch priority
		case 1: return _("Recommended");
		case 2: return "YaST";
		case 3: return _("Documentation");
		// Translators: this refers to patch priority
		case 4: return _("Optional");
		// Translators: this refers to patch priority
		case 5: return _("Other");
	}
	return 0;
}

// Disk

std::vector <std::string> Ypp::getPartitionList()
{
	ZyppDuSet diskUsage = zypp::getZYpp()->diskUsage();
	std::vector <std::string> partitions;
	partitions.reserve (diskUsage.size());
	for (ZyppDuSet::iterator it = diskUsage.begin(); it != diskUsage.end(); it++) {
		const ZyppDu &point = *it;
		if (!point.readonly)
			partitions.push_back (point.dir);
	}
	std::sort (partitions.begin(), partitions.end());
	return partitions;
}

const ZyppDu Ypp::getPartition (const std::string &mount_point)
{
	ZyppDuSet diskUsage = zypp::getZYpp()->diskUsage();
	for (ZyppDuSet::iterator it = diskUsage.begin(); it != diskUsage.end(); it++) {
		const ZyppDu &point = *it;
		if (mount_point == point.dir)
			return point;
	}
	return *zypp::getZYpp()->diskUsage().begin();  // error
}

// Busy

static Ypp::BusyListener *g_busy_listener = 0;
static bool g_busy_running = false;

void Ypp::setBusyListener (Ypp::BusyListener *listener)
{ g_busy_listener = listener; }

struct Ypp::Busy::Impl {
	int cur, size;
	bool showing;
	GTimeVal start_t;

	Impl (int size)
	: cur (0), size (size), showing (false)
	{
		g_get_current_time (&start_t);
		g_busy_running = true;
		g_busy_listener->loading (0);
	}

	~Impl()
	{
		g_busy_running = false;
		g_busy_listener->loading (1);
	}

	void inc()
	{
		cur++;
#if 0
		// shows continuous progress only when it takes more than 1 sec to drive 'N' loops
		if (showing) {
			if ((cur % 500) == 0)
				g_busy_listener->loading (cur / (float) size);
		}
		else if (cur == 499) {
			GTimeVal now_t;
			g_get_current_time (&now_t);
			if (now_t.tv_usec - start_t.tv_usec >= 35*1000 ||
			    now_t.tv_sec - start_t.tv_sec >= 1) {
				showing = true;
			}
		}
#else
		g_busy_listener->loading (cur / (float) size);
#endif
	}
};

Ypp::Busy::Busy (int size) : impl (NULL)
{
	if (g_busy_listener && !g_busy_running)
		impl = new Impl (size);
}

Ypp::Busy::~Busy()
{ delete impl; }

void Ypp::Busy::inc()
{ if (impl) impl->inc(); }

// Interface

std::list <Ypp::SelListener *> g_sel_listeners;
static bool g_transacting = false;

void Ypp::addSelListener (Ypp::SelListener *listener)
{ g_sel_listeners.push_back (listener); }

void Ypp::removeSelListener (Ypp::SelListener *listener)
{ g_sel_listeners.remove (listener); }

void Ypp::notifySelModified()
{
	for (std::list <Ypp::SelListener *>::iterator it = g_sel_listeners.begin();
		 it != g_sel_listeners.end(); it++)
		(*it)->selectableModified();
}

void Ypp::setInterface (Ypp::Interface *interface)
{
	g_interface = interface;
}

Ypp::Interface *Ypp::getInterface() { return g_interface; }

Ypp::Problem::Solution *Ypp::Problem::getSolution (int nb)
{ return (Solution *) g_slist_nth_data ((GSList *) impl, nb); }

bool Ypp::runSolver (bool force)
{
	if (g_transacting) return true;
	if (!g_autoSolver && !force) {
		notifySelModified();
		return true;
	}

	if (g_busy_listener)
		g_busy_listener->loading (0);

	zypp::Resolver_Ptr zResolver = zypp::getZYpp()->resolver();
	bool resolved = false;
	while (true) {
		if (zResolver->resolvePool()) {
			resolved = true;  // no need for user intervention
			break;
		}
		zypp::ResolverProblemList zProblems = zResolver->problems();
		if ((resolved = zProblems.empty())) break;
		if (!g_interface) break;

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

		resolved = g_interface->resolveProblems (problems);
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
				g_slist_free ((GSList *) (*it)->impl);
				delete *it;
			}
			zResolver->applySolutions (choices);
		}
		else
			break;
	}
	if (!resolved)
		zResolver->undo();

	notifySelModified();
	if (g_busy_listener)
		g_busy_listener->loading (1);
	return resolved;
}

void Ypp::setEnableSolver (bool enabled)
{
	g_autoSolver = enabled;
	if (enabled) runSolver();
}

bool Ypp::isSolverEnabled()
{ return g_autoSolver; }

bool Ypp::showPendingLicenses (Ypp::Selectable::Type type)
{
	const zypp::ResKind &kind = Selectable::asKind (type);
	for (ZyppPool::const_iterator it = zyppPool().byKindBegin(kind);
	      it != zyppPool().byKindEnd(kind); it++) {
		ZyppSelectable zsel = (*it);
		switch (zsel->status()) {
			case zypp::ui::S_Install: case zypp::ui::S_AutoInstall:
			case zypp::ui::S_Update: case zypp::ui::S_AutoUpdate:
				if (zsel->candidateObj()) {
					std::string license = zsel->candidateObj()->licenseToConfirm();
					if (!license.empty())
						if (!zsel->hasLicenceConfirmed()) {
							Selectable sel (zsel);
							if (!g_interface->showLicense (sel, license))
								return false;
						}
				}
			default: break;
		}
	}
	return true;
}

void Ypp::startTransactions()
{ g_transacting = true; }

bool Ypp::finishTransactions()
{ g_transacting = false; return runSolver(); }

// Query

struct Ypp::StrMatch::Impl {
	int attrbs;
	std::list <std::string> strings;

	Impl (int attrbs) : attrbs (attrbs) {}
};

Ypp::StrMatch::StrMatch (int attrbs)
: impl (new Impl (attrbs)) {}

Ypp::StrMatch::~StrMatch()
{ delete impl; }

void Ypp::StrMatch::add (const std::string &str)
{ impl->strings.push_back (str); }

bool Ypp::StrMatch::match (Ypp::Selectable &sel)
{
	std::string haystack;
	haystack.reserve (2048);
	if (impl->attrbs & NAME)
		haystack += sel.name();
	if (impl->attrbs & SUMMARY)
		haystack += sel.summary();
	if (impl->attrbs & DESCRIPTION)
		haystack += sel.description (false);

	for (std::list <std::string>::iterator it = impl->strings.begin();
	     it != impl->strings.end(); it++) {
		const std::string &needle = *it;
		if (!strcasestr (haystack.c_str(), needle.c_str()))
			return false;
	}
	return true;
}

Ypp::StatusMatch::StatusMatch (Ypp::StatusMatch::Status status)
: m_status (status) {}

bool Ypp::StatusMatch::match (Selectable &sel)
{
	switch (m_status) {
		case IS_INSTALLED: return sel.isInstalled();
		case NOT_INSTALLED: return !sel.isInstalled();
		case HAS_UPGRADE: return sel.hasUpgrade();
		case IS_LOCKED: return sel.isLocked();
		case TO_MODIFY: return sel.toModify();
	}
	return false;
}

Ypp::PKGroupMatch::PKGroupMatch (YPkgGroupEnum group)
: m_group (group) {}

bool Ypp::PKGroupMatch::match (Selectable &sel)
{
	ZyppPackage pkg = castZyppPackage (sel.zyppSel()->theObj());
	switch (m_group) {
		case YPKG_GROUP_RECOMMENDED: return zypp::PoolItem (pkg).status().isRecommended();
		case YPKG_GROUP_SUGGESTED: return zypp::PoolItem (pkg).status().isSuggested();
		case YPKG_GROUP_ORPHANED: return zypp::PoolItem (pkg).status().isOrphaned();
		case YPKG_GROUP_RECENT:
			if (sel.hasCandidateVersion()) {
				time_t build = static_cast <time_t> (sel.candidate().zyppObj()->buildtime());
				time_t now = time (NULL);
				time_t delta = (now - build) / (60*60*24);  // in days
				return delta <= 7;
			}
			return false;
#if ZYPP_VERSION >= 6030004
		case YPKG_GROUP_MULTIVERSION: return sel.zyppSel()->multiversionInstall();
#endif
		default: return Package (sel).group() == m_group;
	}
	return false;
}

Ypp::RpmGroupMatch::RpmGroupMatch (const std::string &group)
: m_group (group) {}

bool Ypp::RpmGroupMatch::match (Selectable &sel)
{
	std::string pkg_group (Package (sel).rpm_group());
	int len = MIN (m_group.size(), pkg_group.size());
	return m_group.compare (0, len, pkg_group, 0, len) == 0;
}

Ypp::FromCollectionMatch::FromCollectionMatch (Collection &col)
: m_collection (col) {}

bool Ypp::FromCollectionMatch::match (Selectable &sel)
{ return m_collection.contains (sel); }

Ypp::CollectionContainsMatch::CollectionContainsMatch (Ypp::Selectable &sel)
: m_contains (sel) {}

bool Ypp::CollectionContainsMatch::match (Ypp::Selectable &sel)
{
	Collection collection (sel);
	return collection.contains (m_contains);
}

struct Ypp::PoolQuery::Impl {
	ZyppQuery query;
	std::list <Match *> criterias;
	bool filelist_attrb;
	ZyppQuery::Selectable_iterator it;
	bool begin;

	Impl() : filelist_attrb (false), begin (true) {}

	~Impl()
	{
		for (std::list <Match *>::iterator it = criterias.begin();
		     it != criterias.end(); it++)
			delete *it;
	}

	bool match (Ypp::Selectable &sel)
	{  // exclusive
		for (std::list <Match *>::iterator it = criterias.begin();
		     it != criterias.end(); it++)
			if (!(*it)->match (sel))
				return false;
		return true;
	}

	ZyppQuery::Selectable_iterator untilMatch (ZyppQuery::Selectable_iterator it)
	{  // iterates until match -- will return argument if it already matches
		while (it != query.selectableEnd()) {
			Ypp::Selectable sel (*it);
			if (sel.visible() && match (sel)) break;
			it++;
		}
		return it;
	}
};

Ypp::PoolQuery::PoolQuery (Ypp::Selectable::Type type)
: impl (new Impl())
{
	assert (type != Selectable::LANGUAGE);
	if (type != Selectable::ALL)
		impl->query.addKind (Selectable::asKind (type));
}

Ypp::PoolQuery::~PoolQuery()
{ delete impl; }

void Ypp::PoolQuery::setStringMode (bool caseSensitive, Ypp::PoolQuery::MatchType match)
{
	impl->query.setCaseSensitive (caseSensitive);
	switch (match) {
		case CONTAINS: impl->query.setMatchSubstring(); break;
		case EXACT: impl->query.setMatchExact(); break;
		case GLOB: impl->query.setMatchGlob(); break;
		case REGEX: impl->query.setMatchRegex(); break;
	}
}

void Ypp::PoolQuery::addStringAttribute (Ypp::PoolQuery::StringAttribute attrb)
{
	zypp::sat::SolvAttr _attrb;
	impl->filelist_attrb = false;
	switch (attrb) {
		case NAME: _attrb = ZyppAttribute::name; break;
		case SUMMARY: _attrb = ZyppAttribute::summary; break;
		case DESCRIPTION: _attrb = ZyppAttribute::description; break;
		case FILELIST: _attrb = ZyppAttribute::filelist; impl->filelist_attrb = true; break;
		case REQUIRES: _attrb = ZyppAttribute ("solvable:requires"); break;
		case PROVIDES: _attrb = ZyppAttribute ("solvable:provides"); break;
	}
	impl->query.addAttribute (_attrb);
}

void Ypp::PoolQuery::addStringOr (const std::string &str)
{
	if (impl->filelist_attrb && !str.empty())
		impl->query.setFilesMatchFullPath (str[0] == '/');
	impl->query.addString (str);
}

void Ypp::PoolQuery::addRepository (Ypp::Repository &repository)
{ impl->query.addRepo (repository.zyppRepo().info().alias()); }

void Ypp::PoolQuery::addCriteria (Ypp::Match *criteria)
{ impl->criterias.push_back (criteria); }

bool Ypp::PoolQuery::hasNext()
{
	if (impl->begin) {
		impl->it = impl->untilMatch (impl->query.selectableBegin());
		impl->begin = false;
	}
	return impl->it != impl->query.selectableEnd();
}

Ypp::Selectable Ypp::PoolQuery::next()
{
	Ypp::Selectable sel (*impl->it);
	impl->it = impl->untilMatch (++impl->it);
	return sel;
}

int Ypp::PoolQuery::guessSize()
{ return impl->query.size(); }

ZyppQuery &Ypp::PoolQuery::zyppQuery()
{ return impl->query; }

Ypp::Selectable::Type Ypp::PoolQuery::poolType()
{ return Selectable::asType (*impl->query.kinds().begin()); }

struct Ypp::LangQuery::Impl {
	zypp::LocaleSet locales;
	zypp::LocaleSet::const_iterator it;

	Impl() {
		locales = _zyppPool().getAvailableLocales();
		it = locales.begin();
	}
};

Ypp::LangQuery::LangQuery()
: impl (new Impl()) {}

Ypp::LangQuery::~LangQuery()
{ delete impl; }

bool Ypp::LangQuery::hasNext()
{ return impl->it != impl->locales.end(); }

Ypp::Selectable Ypp::LangQuery::next()
{ return Ypp::Selectable (*(impl->it++)); }

int Ypp::LangQuery::guessSize()
{ return impl->locales.size(); }

// List (aggregator)

struct Ypp::List::Impl {
	std::vector <Selectable> vector;
	int refcount;
	Impl() : refcount (1) {}
};

Ypp::List::List (Ypp::Query &query)
: impl (new Impl())
{
	reserve (query.guessSize());
	while (query.hasNext())
		append (query.next());
}

Ypp::List::List (int size)
: impl (new Impl())
{ reserve (size); }

Ypp::List::List (const List &other)
: impl (other.impl)
{ impl->refcount++; }

Ypp::List &Ypp::List::operator= (const List &other)
{
	if (--impl->refcount <= 0) delete impl;
	impl = other.impl;
	impl->refcount++;
	return *this;
}

Ypp::List::~List()
{ if (--impl->refcount <= 0) delete impl; }

Ypp::List Ypp::List::clone()
{
	Ypp::List other (size());
	other.impl->vector = this->impl->vector;
	return other;
}

Ypp::Selectable &Ypp::List::get (int index)
{ return impl->vector[index]; }

int Ypp::List::size()
{ return impl->vector.size(); }

int Ypp::List::count (Match *match)
{
	int count = 0;
	for (std::vector <Selectable>::iterator it = impl->vector.begin();
	     it != impl->vector.end(); it++)
		if (match->match (*it))
			count++;
	return count;
}

int Ypp::List::find (const std::string &name)
{
	for (unsigned int i = 0; i < impl->vector.size(); i++)
		if (name == impl->vector[i].name())
			return i;
	return -1;
}

int Ypp::List::find (Selectable &sel)
{
	for (unsigned int i = 0; i < impl->vector.size(); i++)
		if (sel == impl->vector[i])
			return i;
	return -1;
}

void Ypp::List::reserve (int size)
{ impl->vector.reserve (size); }

void Ypp::List::append (Ypp::Selectable sel)
{ impl->vector.push_back (sel); }

void Ypp::List::install()
{
	startTransactions();
	for (std::vector <Selectable>::iterator it = impl->vector.begin();
	     it != impl->vector.end(); it++)
		it->install();
	if (!finishTransactions())
		undo();
}

void Ypp::List::remove()
{
	startTransactions();
	for (std::vector <Selectable>::iterator it = impl->vector.begin();
	     it != impl->vector.end(); it++)
		it->remove();
	if (!finishTransactions())
		undo();
}

void Ypp::List::lock (bool lock)
{
	startTransactions();
	for (std::vector <Selectable>::iterator it = impl->vector.begin();
	     it != impl->vector.end(); it++)
		it->lock (lock);
	if (!finishTransactions())
		undo();
}

void Ypp::List::undo()
{
	startTransactions();
	for (std::vector <Selectable>::iterator it = impl->vector.begin();
	     it != impl->vector.end(); it++)
		it->undo();
	finishTransactions();
}

static bool installed_order (Ypp::Selectable &a, Ypp::Selectable &b)
{ return (a.isInstalled() ? 0 : 1) < (b.isInstalled() ? 0 : 1); }

static bool utf8_name_order (Ypp::Selectable &a, Ypp::Selectable &b)
{ return g_utf8_collate (a.name().c_str(), b.name().c_str()) < 0; }

static bool name_order (Ypp::Selectable &a, Ypp::Selectable &b)
{ return strcasecmp (a.name().c_str(), b.name().c_str()) < 0; }

static bool size_order (Ypp::Selectable &a, Ypp::Selectable &b)
{ return a.anyVersion().size() < b.anyVersion().size(); }

static std::string get_alias (Ypp::Selectable &a)
{
	if (a.hasCandidateVersion())
		return a.candidate().repository().zyppRepo().alias();
	return "a";  // system
}

static bool repository_order (Ypp::Selectable &a, Ypp::Selectable &b)
{ return get_alias (a) < get_alias(b); }

static bool support_order (Ypp::Selectable &a, Ypp::Selectable &b)
{ return Ypp::Package(a).support() < Ypp::Package(b).support(); }

typedef bool (* order_cb) (Ypp::Selectable &a, Ypp::Selectable &b);

static order_cb _order;
static bool _ascendent;  // proxy between our order callbacks and std::sort's
static bool proxy_order (const Ypp::Selectable &a, const Ypp::Selectable &b)
{ return _order ((Ypp::Selectable &) a, (Ypp::Selectable &) b) == _ascendent; }

void Ypp::List::sort (Ypp::List::SortAttribute attrb, bool ascendent)
{
	if (impl->vector.empty())
		return;

	bool unique_criteria = false;
	switch (attrb) {
		case IS_INSTALLED_SORT: _order = installed_order; break;
		case NAME_SORT:
			// package names are never internationalized, but langs / patterns may be
			if (impl->vector[0].type() != Selectable::PACKAGE)
				_order = utf8_name_order;
			else
				_order = name_order;
			unique_criteria = true;
			break;
		case SIZE_SORT: _order = size_order; unique_criteria = true; break;
		case REPOSITORY_SORT: _order = repository_order; break;
		case SUPPORT_SORT: _order = support_order; break;
	}
	_ascendent = ascendent;
	if (unique_criteria)
		std::sort (impl->vector.begin(), impl->vector.end(), proxy_order);
	else  // many attributes are equal: maintain previous order in such cases
		std::stable_sort (impl->vector.begin(), impl->vector.end(), proxy_order);
}

void Ypp::List::reverse()
{ std::reverse (impl->vector.begin(), impl->vector.end()); }

bool Ypp::List::operator == (const Ypp::List &other) const
{ return this->impl == other.impl; }
bool Ypp::List::operator != (const Ypp::List &other) const
{ return this->impl != other.impl; }

// ListProps

struct Ypp::ListProps::Impl {
	int isInstalled, hasUpgrade, toModify, isLocked, size;
	int canRemove : 2, canLock : 2;

	Impl (List list) {
		isInstalled = hasUpgrade = toModify = isLocked = canRemove = canLock = 0;
		size = list.size();
		for (int  i = 0; i < size; i++) {
			Selectable &sel = list.get (i);
			if (sel.isInstalled()) {
				isInstalled++;
				if (sel.hasUpgrade())
					hasUpgrade++;
			}
			if (sel.toModify())
				toModify++;
			if (sel.isLocked())
				isLocked++;
		}
		if (size) {
			Selectable sel = list.get (0);
			canRemove = sel.canRemove();
			canLock = sel.canLock();
		}
	}
};

Ypp::ListProps::ListProps (Ypp::List &list)
: impl (new Impl (list)) {}

Ypp::ListProps::~ListProps()
{ delete impl; }

bool Ypp::ListProps::isInstalled() const
{ return impl->isInstalled == impl->size; }
bool Ypp::ListProps::isNotInstalled() const
{ return impl->isInstalled == 0; }
bool Ypp::ListProps::hasUpgrade() const
{ return impl->hasUpgrade == impl->size; }
bool Ypp::ListProps::toModify() const
{ return impl->toModify == impl->size; }
bool Ypp::ListProps::isLocked() const
{ return impl->isLocked == impl->size; }
bool Ypp::ListProps::isUnlocked() const
{ return impl->isLocked == 0; }
bool Ypp::ListProps::canRemove() const
{ return impl->canRemove; }
bool Ypp::ListProps::canLock() const
{ return impl->canLock; }

int Ypp::ListProps::isInstalledNb() const
{ return impl->isInstalled; }
int Ypp::ListProps::isNotInstalledNb() const
{ return impl->size - impl->isInstalled; }
int Ypp::ListProps::hasUpgradeNb() const
{ return impl->hasUpgrade; }
int Ypp::ListProps::isLockedNb() const
{ return impl->isLocked; }
int Ypp::ListProps::toModifyNb() const
{ return impl->toModify; }

// General

void Ypp::init()
{
	zyppPool().saveState <zypp::Package>();
	zyppPool().saveState <zypp::Pattern>();
	zyppPool().saveState <zypp::Patch  >();
}

bool Ypp::isModified()
{
	return zyppPool().diffState <zypp::Package>() ||
		zyppPool().diffState <zypp::Pattern>() ||
		zyppPool().diffState <zypp::Patch>();
}

void Ypp::finish()
{
	g_sel_listeners.clear();
	g_interface = 0;
	g_transacting = false;
	g_autoSolver = true;
	g_busy_listener = 0;
	g_busy_running = false;
	g_patternsContent.clear();
	g_languagesContent.clear();
	g_selPatch.clear();
}

