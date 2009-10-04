#include "pkg/YGPackageSelectorPluginImpl.h"

extern "C"
{
    // Important to locate this symbol - see YGPackageSelectorPluginStub.cc
    YGPackageSelectorPluginImpl PSP;
}

