/*
 * Return a string describing everything we know about how this
 * particular binary was built: from what source, for what target
 * platform, using what tools, with what settings, etc.
 */

#include "putty.h"

char *buildinfo(const char *newline)
{
    strbuf *buf = strbuf_new();

    put_fmt(buf, "Build platform: %d-bit %s",
            (int)(CHAR_BIT * sizeof(void *)), BUILDINFO_PLATFORM);

#ifdef __clang_version__
#define FOUND_COMPILER
    put_fmt(buf, "%sCompiler: clang %s", newline, __clang_version__);
#elif defined __GNUC__ && defined __VERSION__
#define FOUND_COMPILER
    put_fmt(buf, "%sCompiler: gcc %s", newline, __VERSION__);
#endif

#if defined _MSC_VER
#ifndef FOUND_COMPILER
#define FOUND_COMPILER
    put_fmt(buf, "%sCompiler: ", newline);
#else
    put_fmt(buf, ", emulating ");
#endif
    put_fmt(buf, "Visual Studio");

#if 0
    /*
     * List of _MSC_VER values and their translations taken from
     * https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
     *
     * The pointless #if 0 branch containing this comment is there so
     * that every real clause can start with #elif and there's no
     * anomalous first clause. That way the patch looks nicer when you
     * add extra ones.
     *
     * Mostly you can tell the version just from _MSC_VER, but in some
     * cases, two different compiler versions have the same _MSC_VER
     * value, and have to be distinguished by _MSC_FULL_VER.
     */
#elif _MSC_VER == 1930
    put_fmt(buf, " 2022 (17.0)");
#elif _MSC_VER == 1929 && _MSC_FULL_VER >= 192930100
    put_fmt(buf, " 2019 (16.11)");
#elif _MSC_VER == 1929
    put_fmt(buf, " 2019 (16.10)");
#elif _MSC_VER == 1928 && _MSC_FULL_VER >= 192829500
    put_fmt(buf, " 2019 (16.9)");
#elif _MSC_VER == 1928
    put_fmt(buf, " 2019 (16.8)");
#elif _MSC_VER == 1927
    put_fmt(buf, " 2019 (16.7)");
#elif _MSC_VER == 1926
    put_fmt(buf, " 2019 (16.6)");
#elif _MSC_VER == 1925
    put_fmt(buf, " 2019 (16.5)");
#elif _MSC_VER == 1924
    put_fmt(buf, " 2019 (16.4)");
#elif _MSC_VER == 1923
    put_fmt(buf, " 2019 (16.3)");
#elif _MSC_VER == 1922
    put_fmt(buf, " 2019 (16.2)");
#elif _MSC_VER == 1921
    put_fmt(buf, " 2019 (16.1)");
#elif _MSC_VER == 1920
    put_fmt(buf, " 2019 (16.0)");
#elif _MSC_VER == 1916
    put_fmt(buf, " 2017 version 15.9");
#elif _MSC_VER == 1915
    put_fmt(buf, " 2017 version 15.8");
#elif _MSC_VER == 1914
    put_fmt(buf, " 2017 version 15.7");
#elif _MSC_VER == 1913
    put_fmt(buf, " 2017 version 15.6");
#elif _MSC_VER == 1912
    put_fmt(buf, " 2017 version 15.5");
#elif _MSC_VER == 1911
    put_fmt(buf, " 2017 version 15.3");
#elif _MSC_VER == 1910
    put_fmt(buf, " 2017 RTW (15.0)");
#elif _MSC_VER == 1900
    put_fmt(buf, " 2015 (14.0)");
#elif _MSC_VER == 1800
    put_fmt(buf, " 2013 (12.0)");
#elif _MSC_VER == 1700
    put_fmt(buf, " 2012 (11.0)");
#elif _MSC_VER == 1600
    put_fmt(buf, " 2010 (10.0)");
#elif _MSC_VER == 1500
    put_fmt(buf, " 2008 (9.0)");
#elif _MSC_VER == 1400
    put_fmt(buf, " 2005 (8.0)");
#elif _MSC_VER == 1310
    put_fmt(buf, " .NET 2003 (7.1)");
#elif _MSC_VER == 1300
    put_fmt(buf, " .NET 2002 (7.0)");
#elif _MSC_VER == 1200
    put_fmt(buf, " 6.0");
#else
    put_fmt(buf, ", unrecognised version");
#endif
    put_fmt(buf, ", _MSC_VER=%d", (int)_MSC_VER);
#ifdef _MSC_FULL_VER
    put_fmt(buf, ", _MSC_FULL_VER=%d", (int)_MSC_FULL_VER);
#endif
#endif

#ifdef BUILDINFO_GTK
    {
        char *gtk_buildinfo = buildinfo_gtk_version();
        if (gtk_buildinfo) {
            put_fmt(buf, "%sCompiled against GTK version %s",
                    newline, gtk_buildinfo);
            sfree(gtk_buildinfo);
        }
    }
#endif
#if defined _WINDOWS
    {
        int echm = has_embedded_chm();
        if (echm >= 0)
            put_fmt(buf, "%sEmbedded HTML Help file: %s", newline,
                    echm ? "yes" : "no");
    }
#endif

#if defined _WINDOWS && defined MINEFIELD
    put_fmt(buf, "%sBuild option: MINEFIELD", newline);
#endif
#ifdef NO_IPV6
    put_fmt(buf, "%sBuild option: NO_IPV6", newline);
#endif
#ifdef NO_GSSAPI
    put_fmt(buf, "%sBuild option: NO_GSSAPI", newline);
#endif
#ifdef STATIC_GSSAPI
    put_fmt(buf, "%sBuild option: STATIC_GSSAPI", newline);
#endif
#ifdef UNPROTECT
    put_fmt(buf, "%sBuild option: UNPROTECT", newline);
#endif
#ifdef FUZZING
    put_fmt(buf, "%sBuild option: FUZZING", newline);
#endif
#ifdef DEBUG
    put_fmt(buf, "%sBuild option: DEBUG", newline);
#endif

    put_fmt(buf, "%sSource commit: %s", newline, commitid);

    return strbuf_to_str(buf);
}
