/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>

YUI * createUI( bool withThreads )
{
    static YGUI *_ui = 0;
    
    if ( ! _ui )
    {
        _ui = new YGUI( withThreads );
    }

    return _ui;
}

