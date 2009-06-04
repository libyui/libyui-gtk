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

  File:     YGPackageSelectorPluginIf.h

  Author:   Martin Kudlvasr <mkudlvasr@suse.cz>


/-*/

#ifndef YGPackageSelectorPluginIf_h
#define YGPackageSelectorPluginIf_h

#include <YPackageSelectorPlugin.h>

class YGPackageSelectorPluginIf
{

public:

    virtual ~YGPackageSelectorPluginIf() {}

    virtual YPackageSelector *createPackageSelector( YWidget *parent, long modeFlags ) = 0 ;
};


#endif
