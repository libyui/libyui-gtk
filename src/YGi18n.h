/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGi18n_h
#define YGi18n_h

#include <libintl.h>
#define TEXTDOMAIN "gtk"

static inline const char *_(const char *msgid)
{ return dgettext (TEXTDOMAIN, msgid); }

#ifndef YGI18N_C
static inline const char * _(const char * msgid1, const char *msgid2, unsigned long int n)
{ return dngettext(TEXTDOMAIN, msgid1, msgid2, n); }
#endif

#endif /*YGi18n_h*/

