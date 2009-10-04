#ifndef YGPACKAGE_SELECTOR_PLUGIN_IF
#define YGPACKAGE_SELECTOR_PLUGIN_IF

struct YPackageSelector;
struct YWidget;

class YGPackageSelectorPluginIf
{

  public:

    virtual ~YGPackageSelectorPluginIf() {}

    virtual YPackageSelector *createPackageSelector( YWidget *parent, long modeFlags ) = 0 ;
    virtual YWidget *createPatternSelector( YWidget * parent, long modeFlags ) = 0;
    virtual YWidget *createSimplePatchSelector( YWidget * parent, long modeFlags ) = 0;
};

#endif /*YGPACKAGE_SELECTOR_PLUGIN_IF*/

