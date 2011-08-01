/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include "pkg/YGPackageSelectorPluginImpl.h"

extern "C"
{
	// Important to locate this symbol - see YGPackageSelectorPluginStub.cc
	YGPackageSelectorPluginImpl PSP;
}

