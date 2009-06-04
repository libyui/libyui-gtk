/*---------------------------------------------------------------------\
|                                                                      |
|                       __   __    ____ _____ ____                     |
|                       \ \ / /_ _/ ___|_   _|___ \                    |
|                        \ V / _` \___ \ | |   __) |                   |
|                         | | (_| |___) || |  / __/                    |
|                         |_|\__,_|____/ |_| |_____|                   |
|                                                                      |
|                                core system                           |
|                                                    (c) SuSE Linux AG |
\----------------------------------------------------------------------/

  File:		NCPackageSelectorPluginStub.cc

  Author:	Martin Kudlvasr <mkudlvasr@suse.cz>


/-*/

#include "YGPackageSelectorPluginStub.h"

#define YUILogComponent "gtk"
#include "YUILog.h"
#include "YGWidget.h"
#include "YGLabel.h"
#include "YGDialog.h"
#include "YGPackageSelectorPluginIf.h"
#include "YGUI.h"

#define PLUGIN_BASE_NAME "gtk_pkg"


YGPackageSelectorPluginStub::YGPackageSelectorPluginStub()
    : YPackageSelectorPlugin( PLUGIN_BASE_NAME )
{
	if ( success() )
	{
		yuiMilestone() << "Loaded " << PLUGIN_BASE_NAME
		<< " plugin successfully from " << pluginLibFullPath()
		<< endl;
	}


	impl = ( YGPackageSelectorPluginIf* ) locateSymbol( "PSP" );

    if ( !impl )
	YUI_THROW( YUIPluginException( PLUGIN_BASE_NAME ) );

}


YGPackageSelectorPluginStub::~YGPackageSelectorPluginStub()
{
    // NOP
}


YPackageSelector * YGPackageSelectorPluginStub::createPackageSelector( YWidget * parent,
                                                                       long modeFlags )
{
	return impl->createPackageSelector( parent, modeFlags );
}

YPackageSelector * YGWidgetFactory::createPackageSelector(YWidget * parent, long modeFlags)
{
	YGPackageSelectorPluginStub * plugin = YGApplication::packageSelectorPlugin();
	YUI_CHECK_PTR( plugin );

	YPackageSelector * pkgSel = plugin->createPackageSelector( parent, modeFlags );
	YUI_CHECK_NEW( pkgSel );

	return pkgSel;
}
