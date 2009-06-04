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

  File:	      YGPackageSelectorPluginStub.h

  Author:     Martin Kudlvasr <mkudlvasr@suse.cz>

/-*/

#ifndef YGPackageSelectorPluginStub_h
#define YGPackageSelectorPluginStub_h

#include <YPackageSelectorPlugin.h>
#include <YDialog.h>
#include <YEvent.h>

#include "YGPackageSelectorPluginIf.h"

/**
 * Simplified access to the Gtk UI's package selector plugin.
 **/

class YGPackageSelectorPluginIf;

class YGPackageSelectorPluginStub: public YPackageSelectorPlugin
{
public:

    /**
     * Constructor: Load the plugin library for the Gtk package selector.
     **/
    YGPackageSelectorPluginStub();

    /**
     * Destructor. Calls dlclose() which will unload the plugin library if it
     * is no longer used, i.e. if the reference count dlopen() uses reaches 0.
     **/
    virtual ~YGPackageSelectorPluginStub();

    /**
     * Create a package selector.
     * Implemented from YPackageSelectorPlugin.
     *
     * This might return 0 if the plugin lib could not be loaded or if the
     * appropriate symbol could not be located in the plugin lib.
     **/
    virtual YPackageSelector * createPackageSelector( YWidget * parent,
                                                      long      modeFlags );

    
    YGPackageSelectorPluginIf *impl;
};


#endif // YGPackageSelectorPluginStub_h
