/* Copyright (C) 1999-2019 Free Software Foundation, Inc.
   This file is part of the GNU LIBICONV Library.

   The GNU LIBICONV Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   The GNU LIBICONV Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU LIBICONV Library; see the file COPYING.LIB.
   If not, see <https://www.gnu.org/licenses/>.  */

/* When installed, this file is called "iconv.h". */

#ifndef _LIBICONV_H
#define _LIBICONV_H

#define _LIBICONV_VERSION 0x0110    /* version number: (major<<8) + minor */

#ifdef BUILDING_LIBICONV
#define LIBICONV_DLL_EXPORTED __declspec(dllexport)
#elif LIBICONV_DLL
#define LIBICONV_DLL_EXPORTED __declspec(dllimport)
#else
#define LIBICONV_DLL_EXPORTED
#endif
extern LIBICONV_DLL_EXPORTED int _libiconv_version;

/* We would like to #include any system header file which could define
   iconv_t, 1. in order to eliminate the risk that the user gets compilation
   errors because some other system header file includes /usr/include/iconv.h
   which defines iconv_t or declares iconv after this file, 2. when compiling
   for LIBICONV_PLUG, we need the proper iconv_t type in order to produce
   binary compatible code.
   But gcc's #include_next is not portable. Thus, once libiconv's iconv.h
   has been installed in /usr/local/include, there is no way any more to
   include the original /usr/include/iconv.h. We simply have to get away
   without it.
   Ad 1. The risk that a system header file does
   #include "iconv.h"  or  #include_next "iconv.h"
   is small. They all do #include <iconv.h>.
   Ad 2. The iconv_t type is a pointer type in all cases I have seen. (It
   has to be a scalar type because (iconv_t)(-1) is a possible return value
   from iconv_open().) */

/* Define iconv_t ourselves. */
#undef iconv_t
#define iconv_t libiconv_t
typedef void* iconv_t;

/* Get size_t declaration.
   Get wchar_t declaration if it exists. */
#include <stddef.h>

/* Get errno declaration and values. */
#include <errno.h>
/* Some systems, like SunOS 4, don't have EILSEQ. Some systems, like BSD/OS,
   have EILSEQ in a different header.  On these systems, define EILSEQ
   ourselves. */
#ifndef EILSEQ
#define EILSEQ 
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Allocates descriptor for code conversion from encoding `fromcode' to
   encoding `tocode'. */
#ifndef LIBICONV_PLUG
#define iconv_open libiconv_open
#endif
extern LIBICONV_DLL_EXPORTED iconv_t iconv_open (const char* tocode, const char* fromcode);

/* Converts, using conversion descriptor `cd', at most `*inbytesleft' bytes
   starting at `*inbuf', writing at most `*outbytesleft' bytes starting at
   `*outbuf'.
   Decrements `*inbytesleft' and increments `*inbuf' by the same amount.
   Decrements `*outbytesleft' and increments `*outbuf' by the same amount. */
#ifndef LIBICONV_PLUG
#define iconv libiconv
#endif
extern LIBICONV_DLL_EXPORTED size_t iconv (iconv_t cd,  char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);

/* Frees resources allocated for conversion descriptor `cd'. */
#ifndef LIBICONV_PLUG
#define iconv_close libiconv_close
#endif
extern LIBICONV_DLL_EXPORTED int iconv_close (iconv_t cd);

#ifdef __cplusplus
}
#endif

#ifndef LIBICONV_PLUG

/* Nonstandard extensions. */

#if USE_MBSTATE_T
#if BROKEN_WCHAR_H
/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.  */
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#endif
#include <wchar.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* A type that holds all memory needed by a conversion descriptor.
   A pointer to such an object can be used as an iconv_t. */
typedef struct {
  void* dummy1[28];
#if USE_MBSTATE_T
  mbstate_t dummy2;
#endif
} iconv_allocation_t;

/* Allocates descriptor for code conversion from encoding ‘fromcode’ to
   encoding ‘tocode’ into preallocated memory. Returns an error indicator
   (0 or -1 with errno set). */
#define iconv_open_into libiconv_open_into
extern int iconv_open_into (const char* tocode, const char* fromcode,
                            iconv_allocation_t* resultp);

/* Control of attributes. */
#define iconvctl libiconvctl
extern LIBICONV_DLL_EXPORTED int iconvctl (iconv_t cd, int request, void* argument);

/* Hook performed after every successful conversion of a Unicode character. */
typedef void (*iconv_unicode_char_hook) (unsigned int uc, void* data);
/* Hook performed after every successful conversion of a wide character. */
typedef void (*iconv_wide_char_hook) (wchar_t wc, void* data);
/* Set of hooks. */
struct iconv_hooks {
  iconv_unicode_char_hook uc_hook;
  iconv_wide_char_hook wc_hook;
  void* data;
};

/* Fallback function.  Invoked when a small number of bytes could not be
   converted to a Unicode character.  This function should process all
   bytes from inbuf and may produce replacement Unicode characters by calling
   the write_replacement callback repeatedly.  */
typedef void (*iconv_unicode_mb_to_uc_fallback)
             (const char* inbuf, size_t inbufsize,
              void (*write_replacement) (const unsigned int *buf, size_t buflen,
                                         void* callback_arg),
              void* callback_arg,
              void* data);
/* Fallback function.  Invoked when a Unicode character could not be converted
   to the target encoding.  This function should process the character and
   may produce replacement bytes (in the target encoding) by calling the
   write_replacement callback repeatedly.  */
typedef void (*iconv_unicode_uc_to_mb_fallback)
             (unsigned int code,
              void (*write_replacement) (const char *buf, size_t buflen,
                                         void* callback_arg),
              void* callback_arg,
              void* data);
#if 0
/* Fallback function.  Invoked when a number of bytes could not be converted to
   a wide character.  This function should process all bytes from inbuf and may
   produce replacement wide characters by calling the write_replacement
   callback repeatedly.  */
typedef void (*iconv_wchar_mb_to_wc_fallback)
             (const char* inbuf, size_t inbufsize,
              void (*write_replacement) (const wchar_t *buf, size_t buflen,
                                         void* callback_arg),
              void* callback_arg,
              void* data);
/* Fallback function.  Invoked when a wide character could not be converted to
   the target encoding.  This function should process the character and may
   produce replacement bytes (in the target encoding) by calling the
   write_replacement callback repeatedly.  */
typedef void (*iconv_wchar_wc_to_mb_fallback)
             (wchar_t code,
              void (*write_replacement) (const char *buf, size_t buflen,
                                         void* callback_arg),
              void* callback_arg,
              void* data);
#else
/* If the wchar_t type does not exist, these two fallback functions are never
   invoked.  Their argument list therefore does not matter.  */
typedef void (*iconv_wchar_mb_to_wc_fallback) ();
typedef void (*iconv_wchar_wc_to_mb_fallback) ();
#endif
/* Set of fallbacks. */
struct iconv_fallbacks {
  iconv_unicode_mb_to_uc_fallback mb_to_uc_fallback;
  iconv_unicode_uc_to_mb_fallback uc_to_mb_fallback;
  iconv_wchar_mb_to_wc_fallback mb_to_wc_fallback;
  iconv_wchar_wc_to_mb_fallback wc_to_mb_fallback;
  void* data;
};

/* Requests for iconvctl. */
#define ICONV_TRIVIALP            0  /* int *argument */
#define ICONV_GET_TRANSLITERATE   1  /* int *argument */
#define ICONV_SET_TRANSLITERATE   2  /* const int *argument */
#define ICONV_GET_DISCARD_ILSEQ   3  /* int *argument */
#define ICONV_SET_DISCARD_ILSEQ   4  /* const int *argument */
#define ICONV_SET_HOOKS           5  /* const struct iconv_hooks *argument */
#define ICONV_SET_FALLBACKS       6  /* const struct iconv_fallbacks *argument */

/* Listing of locale independent encodings. */
#define iconvlist libiconvlist
extern LIBICONV_DLL_EXPORTED void iconvlist (int (*do_one) (unsigned int namescount,
                                      const char * const * names,
                                      void* data),
                       void* data);

/* Canonicalize an encoding name.
   The result is either a canonical encoding name, or name itself. */
extern LIBICONV_DLL_EXPORTED const char * iconv_canonicalize (const char * name);

/* Support for relocatable packages.  */

/* Sets the original and the current installation prefix of the package.
   Relocation simply replaces a pathname starting with the original prefix
   by the corresponding pathname with the current prefix instead.  Both
   prefixes should be directory names without trailing slash (i.e. use ""
   instead of "/").  */
extern LIBICONV_DLL_EXPORTED void libiconv_set_relocation_prefix (const char *orig_prefix,
					    const char *curr_prefix);


#ifdef __cplusplus
}
#endif

#endif


#endif /* _LIBICONV_H */
