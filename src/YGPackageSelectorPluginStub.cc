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

bool show_find_pane = false, use_buttons = false, show_novelty_filter = false;
bool YGUI::pkgSelectorParse (const char *arg)
{
	if (!strcmp (arg, "find-pane"))
		show_find_pane = true;
	else if (!strcmp (arg, "buttons"))
		use_buttons = true;
	else if (!strcmp (arg, "novelty-filter"))
		show_novelty_filter = true;
	else return false;
	return true;
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

