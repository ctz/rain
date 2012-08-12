/* this file defines which wxConfig is used. currently
 * wxFileConfig is forced upon all platforms */

#undef wxUSE_CONFIG_NATIVE
#define wxUSE_CONFIG_NATIVE 0
#include <wx/confbase.h>
#include <wx/fileconf.h>

/* to use wxRegConfig on windows uncomment the following
 * and comment the preceeding */

/*#include <wx/config.h>*/