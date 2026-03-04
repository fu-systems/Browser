/*
 * Pane — Platform Compatibility Header
 *
 * Provides POSIX-to-MSVC function mappings for Windows builds.
 */

#ifndef PANE_COMPAT_H
#define PANE_COMPAT_H

#ifdef _MSC_VER
  /* MSVC doesn't have <strings.h> or POSIX string functions. */
  #include <string.h>
  #define strcasecmp   _stricmp
  #define strncasecmp  _strnicmp
  #define strdup       _strdup
#else
  #include <strings.h>
#endif

#endif /* PANE_COMPAT_H */
