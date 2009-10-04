#ifndef YGPACKAGE_SELECTOR_PLUGIN_IMPL_H
#define YGPACKAGE_SELECTOR_PLUGIN_IMPL_H

struct YPackageSelector;
struct YWidget;

#include "YGPackageSelectorPluginIf.h"

class YGPackageSelectorPluginImpl : public YGPackageSelectorPluginIf
{

  public:

    virtual ~YGPackageSelectorPluginImpl() {}

    virtual YPackageSelector * createPackageSelector( YWidget *	parent, long modeFlags);

    virtual YWidget *createPatternSelector( YWidget * parent, long modeFlags ) { return 0; }
    virtual YWidget *createSimplePatchSelector( YWidget * parent, long modeFlags ) { return 0; }
};

#endif /*YGPACKAGE_SELECTOR_PLUGIN_IMPL_H*/

