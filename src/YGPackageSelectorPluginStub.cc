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

class YGPackageSelectorPluginStub : public YPackageSelectorPlugin
{
YGPackageSelectorPluginIf *impl;

public:
	YGPackageSelectorPluginStub()
	: YPackageSelectorPlugin (PLUGIN_BASE_NAME)
	{
		if (success())
			yuiMilestone() << "Loaded " << PLUGIN_BASE_NAME
				<< " plugin successfully from " << pluginLibFullPath() << std::endl;
		impl = (YGPackageSelectorPluginIf *) locateSymbol ("PSP");
		if (!impl)
			yuiError() << "Plugin " << PLUGIN_BASE_NAME
				<< " does not provide PSP symbol" << std::endl;
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

