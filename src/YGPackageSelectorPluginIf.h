/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGPACKAGE_SELECTOR_PLUGIN_IF
#define YGPACKAGE_SELECTOR_PLUGIN_IF

struct YPackageSelector;
struct YWidget;

class YGPackageSelectorPluginIf
{
public:
	virtual YPackageSelector *createPackageSelector (YWidget *parent, long modeFlags) = 0;
	virtual YWidget *createPatternSelector (YWidget *parent, long modeFlags) = 0;
	virtual YWidget *createSimplePatchSelector (YWidget *parent, long modeFlags) = 0;
};

#endif /*YGPACKAGE_SELECTOR_PLUGIN_IF*/

