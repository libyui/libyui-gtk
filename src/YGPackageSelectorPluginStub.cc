/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include "YGUI.h"
#include "YGPackageSelectorPluginIf.h"

#define PLUGIN_BASE_NAME "gtk_pkg"
#include <YPackageSelectorPlugin.h>
#include <YDialog.h>
#include <YEvent.h>
#include <string.h>

bool search_entry_side = true, search_entry_top = false, dynamic_sidebar = false,
	categories_side = true, categories_top = false, status_side = false,
	status_top = false, status_tabs = true, status_tabs_as_actions = false,
	undo_side = false, undo_tab = true, undo_old_style = false, status_col = false,
	action_col = true, action_col_as_button = true, action_col_as_check = false,
	version_col = true, single_line_rows = false, details_start_hide = true,
	toolbar_top = false, toolbar_yast = false, arrange_by = false;

struct Arg {
	const char *arg;
	bool *var;
};
Arg arguments[] = {
	{ "search-entry-side", &search_entry_side },
	{ "search-entry-top", &search_entry_top },
	{ "dynamic-sidebar", &dynamic_sidebar },
	{ "categories-side", &categories_side },
	{ "categories-top", &categories_top },
	{ "status-side", &status_side },
	{ "status-top", &status_top },
	{ "status-tabs", &status_tabs },
	{ "status-tabs-as-actions", &status_tabs_as_actions },
	{ "undo-side", &undo_side },
	{ "undo-tab", &undo_tab },
	{ "undo-old-style", &undo_old_style },
	{ "status-col", &status_col },
	{ "action-col", &action_col },
	{ "action-col-as-button", &action_col_as_button },
	{ "action-col-as-check", &action_col_as_check },
	{ "version-col", &version_col },
	{ "single-line-rows", &single_line_rows },
	{ "details-start-hide", &details_start_hide },
	{ "toolbar-top", &toolbar_top },
	{ "toolbar-yast", &toolbar_yast },
	{ "arrange-by", &arrange_by },
};
static const int arguments_nb = sizeof (arguments) / sizeof (Arg);

bool YGUI::pkgSelectorParse (const char *arg)
{
	if (!strcmp (arg, "help-pkg")) {
		printf ("sw_single gtk [OPTIONS]:\n");
		for (int i = 0; i < arguments_nb; i++)
			printf ("\t--%s=y/n\t\t(default: %c)\n",
				arguments[i].arg, *arguments[i].var ? 'y' : 'n');
		exit (0);
	}
	for (int i = 0; i < arguments_nb; i++) {
		int arg_len = strlen (arguments[i].arg);
		if (!strncmp (arg, arguments[i].arg, arg_len) && arg[arg_len] == '=') {
			*arguments[i].var = arg[arg_len+1] == 'y';
//fprintf (stderr, "found '%s' as '%s' = '%d'\n", arg, arguments[i].arg, *arguments[i].var);
			return true;
		}
	}
	return false;
}

void YGUI::pkgSelectorSize (int *width, int *height)
{ *width = 700; *height = 750; }

class YGPackageSelectorPluginStub : public YPackageSelectorPlugin
{
YGPackageSelectorPluginIf *impl;

public:
	YGPackageSelectorPluginStub()
	: YPackageSelectorPlugin (PLUGIN_BASE_NAME)
	{
		if (success())
			yuiMilestone() << "Loaded " << PLUGIN_BASE_NAME
				<< " plugin successfully from " << pluginLibFullPath() << endl;
		impl = (YGPackageSelectorPluginIf *) locateSymbol ("PSP");
		if (!impl)
			yuiError() << "Plugin " << PLUGIN_BASE_NAME
				<< " does not provide PSP symbol" << endl;
	}

	static YGPackageSelectorPluginStub *get()
	{
    	static YGPackageSelectorPluginStub *plugin = 0;
		if (!plugin)
	        // deliberately keep plugin open
    	    plugin = new YGPackageSelectorPluginStub();
		return plugin;
	}

	virtual YPackageSelector *createPackageSelector (YWidget *parent, long modeFlags)
	{
		if (!impl)
			YUI_THROW (YUIPluginException (PLUGIN_BASE_NAME));
		return impl->createPackageSelector (parent, modeFlags);
	}

	virtual YWidget *createPatternSelector (YWidget *parent, long modeFlags)
	{
		if (!impl)
			YUI_THROW (YUIPluginException (PLUGIN_BASE_NAME));
		return impl->createPatternSelector (parent, modeFlags);
	}

	virtual YWidget *createSimplePatchSelector (YWidget *parent, long modeFlags)
	{
		if (!impl)
			YUI_THROW (YUIPluginException (PLUGIN_BASE_NAME));
		return impl->createSimplePatchSelector (parent, modeFlags);
	}
};

// YWidgetFactory

YPackageSelector* YGWidgetFactory::createPackageSelector (YWidget* parent, long modeFlags)
{
	YGPackageSelectorPluginStub *plugin = YGPackageSelectorPluginStub::get();
	if (plugin)
    	return plugin->createPackageSelector (parent, modeFlags);
    return NULL;
}

// YOptionalWidgetFactory

YWidget* YGOptionalWidgetFactory::createPatternSelector (YWidget* parent, long modeFlags)
{
	YGPackageSelectorPluginStub *plugin = YGPackageSelectorPluginStub::get();
	if (plugin)
    	return plugin->createPatternSelector (parent, modeFlags);
    return NULL;
}

YWidget* YGOptionalWidgetFactory::createSimplePatchSelector (YWidget* parent, long modeFlags)
{
	YGPackageSelectorPluginStub *plugin = YGPackageSelectorPluginStub::get();
	if (plugin)
    	return plugin->createSimplePatchSelector (parent, modeFlags);
    return NULL;
}

