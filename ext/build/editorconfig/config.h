/*
 * Copyright (c) 2011-2012 EditorConfig Team
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifndef UNIX
/* #undef UNIX */
#endif

#ifndef WIN32
#define WIN32
#endif

/* #undef HAVE_STRCASECMP */
#define HAVE_STRDUP
#define HAVE_STRICMP
/* #undef HAVE_STRNDUP */
#define HAVE_STRLWR

#if _MSC_VER >= 1800
#define HAVE__BOOL
#endif

#define HAVE_CONST

#ifndef HAVE_CONST
# define const
#endif

/* #undef CMAKE_COMPILER_IS_GNUCC */
#define MSVC

#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8

/* For gcc, we define _GNU_SOURCE to use gcc extensions */
#ifdef CMAKE_COMPILER_IS_GNUCC
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif /* _GNU_SOURCE */
#endif /* CMAKE_COMPILER_IS_GNUCC */

#ifdef MSVC
# define _CRT_SECURE_NO_WARNINGS 1
# pragma warning(disable: 4996)
#endif

/* _Bool type */
#ifndef HAVE__BOOL
# define _Bool signed char
#endif

#define EC_VERSION_MAJOR 0
#define EC_VERSION_MINOR 12
#define EC_VERSION_PATCH 4
#define EC_VERSION_SUFFIX ""


#endif /* !__CONFIG_H__ */
