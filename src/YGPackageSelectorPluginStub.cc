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
{ *width = 700; *height = 800; }

class YGPackageSelectorPluginStub: public YPackageSelectorPlugin
{
public:
    /**
     * Constructor: Load the plugin library for the package selector.
     **/
    YGPackageSelectorPluginStub();

    /**
     * Destructor.
     **/
    virtual ~YGPackageSelectorPluginStub();

    /**
     * Create a package selector.
     * Implemented from YPackageSelectorPlugin.
     *
     * This might return 0 if the plugin lib could not be loaded or if the
     * appropriate symbol could not be located in the plugin lib.
     **/
    virtual YPackageSelector * createPackageSelector( YWidget *	parent,
						      long	modeFlags );

    /**
     * Create a pattern selector (optional widget).
     **/
    virtual YWidget * createPatternSelector( YWidget *	parent,
                                             long	modeFlags );

    /**
     * Create a simple patch selector (optional widget).
     **/
    virtual YWidget * createSimplePatchSelector( YWidget * parent,
                                                 long	   modeFlags );


    YGPackageSelectorPluginIf * impl;
};

YGPackageSelectorPluginStub::YGPackageSelectorPluginStub()
    : YPackageSelectorPlugin( PLUGIN_BASE_NAME )
{
    if ( success() )
    {
	yuiMilestone() << "Loaded " << PLUGIN_BASE_NAME
                       << " plugin successfully from " << pluginLibFullPath()
                       << endl;
    }


    impl = (YGPackageSelectorPluginIf*) locateSymbol("PSP");
    
    if ( ! impl )
    {
        yuiError() << "Plugin " << PLUGIN_BASE_NAME << " does not provide PSP symbol" << endl;
    }
}


YGPackageSelectorPluginStub::~YGPackageSelectorPluginStub()
{
    // NOP
}


YPackageSelector *
YGPackageSelectorPluginStub::createPackageSelector( YWidget * parent, long modeFlags )
{
    if ( ! impl )
	YUI_THROW( YUIPluginException( PLUGIN_BASE_NAME ) );
    
    return impl->createPackageSelector( parent, modeFlags );
}


YWidget *
YGPackageSelectorPluginStub::createPatternSelector( YWidget * parent, long modeFlags )
{
    if ( ! impl )
	YUI_THROW( YUIPluginException( PLUGIN_BASE_NAME ) );
    
    return impl->createPatternSelector( parent, modeFlags );
}


YWidget *
YGPackageSelectorPluginStub::createSimplePatchSelector( YWidget * parent, long modeFlags )
{
    if ( ! impl )
	YUI_THROW( YUIPluginException( PLUGIN_BASE_NAME ) );
    
    return impl->createSimplePatchSelector( parent, modeFlags );
}

// YGWidgetFactory

static YGPackageSelectorPluginStub *packageSelectorPlugin()
{
    static YGPackageSelectorPluginStub * plugin = 0;
    if ( ! plugin )
    {
        plugin = new YGPackageSelectorPluginStub();

        // This is a deliberate memory leak: If an application requires a
        // PackageSelector, it is a package selection application by
        // definition. In this case, the ncurses_pkg plugin is intentionally
        // kept open to avoid repeated start-up cost of the plugin and libzypp.
    }

    return plugin;
}


YPackageSelector*
YGWidgetFactory::createPackageSelector(YWidget* parent, long modeFlags)
{
    YGPackageSelectorPluginStub * plugin = packageSelectorPlugin();
    YPackageSelector * pkgSel = plugin->createPackageSelector( parent, modeFlags );
    return pkgSel;
}

#if 0
YWidget *
YGOptionalWidgetFactory::createPatternSelector(YWidget* parent, long modeFlags)
{
    YGPackageSelectorPluginStub * plugin = packageSelectorPlugin();
    if ( plugin )
        return plugin->createPatternSelector( parent, modeFlags );
    else
        return 0;
}

YWidget *
YGOptionalWidgetFactory::createSimplePatchSelector(YWidget* parent, long modeFlags)
{
    YGPackageSelectorPluginStub * plugin = packageSelectorPlugin();
    if ( plugin )
        return plugin->createSimplePatchSelector( parent, modeFlags );
    else
        return 0;
}
#endif

