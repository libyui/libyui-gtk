/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

  File:		YGi18n.h

  Author:	Jiri Srain <jsrain@suse.cz>

/-*/

// -*- c++ -*-

#ifndef YGi18n_h
#define YGi18n_h

#include <libintl.h>

#define TEXTDOMAIN "yast2-gtk"

static inline const char * _( const char * msgid )
{
	return ( !msgid || !*msgid ) ? "" : dgettext( TEXTDOMAIN, msgid );
}

#ifndef YGI18N_C
static inline const char * _( const char * msgid1, const char * msgid2, unsigned long int n )
{
	return dngettext( TEXTDOMAIN, msgid1, msgid2, n );
}
#endif

#endif // YGi18n_h
