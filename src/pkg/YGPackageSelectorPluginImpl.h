/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGPACKAGE_SELECTOR_PLUGIN_IMPL_H
#define YGPACKAGE_SELECTOR_PLUGIN_IMPL_H

struct YPackageSelector;
struct YWidget;

#include "YGPackageSelectorPluginIf.h"

class YGPackageSelectorPluginImpl : public YGPackageSelectorPluginIf
{
public:
	virtual YPackageSelector *createPackageSelector (YWidget *parent, long modeFlags);
	virtual YWidget *createPatternSelector (YWidget *parent, long modeFlags);
	virtual YWidget *createSimplePatchSelector (YWidget *parent, long modeFlags);
};

#endif /*YGPACKAGE_SELECTOR_PLUGIN_IMPL_H*/

