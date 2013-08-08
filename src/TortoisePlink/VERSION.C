/*
 * PuTTY version numbering
 */

#define STR1(x) #x
#define STR(x) STR1(x)

#ifdef INCLUDE_EMPTY_H
/*
 * Horrible hack to force version.o to be rebuilt unconditionally in
 * the automake world: empty.h is an empty header file, created by the
 * makefile and forcibly updated every time make is run. Including it
 * here causes automake to track it as a dependency, which will cause
 * version.o to be rebuilt too.
 *
 * The space between # and include causes mkfiles.pl's dependency
 * scanner (for all other makefile types) to ignore this include,
 * which is correct because only the automake makefile passes
 * -DINCLUDE_EMPTY_H to enable it.
 */
# include "empty.h"
#endif

#if defined SNAPSHOT

#if defined SVN_REV
#define SNAPSHOT_TEXT STR(SNAPSHOT) ":r" STR(SVN_REV)
#else
#define SNAPSHOT_TEXT STR(SNAPSHOT)
#endif

char ver[] = "Development snapshot " SNAPSHOT_TEXT;
char sshver[] = "PuTTY-Snapshot-" SNAPSHOT_TEXT;

#undef SNAPSHOT_TEXT

#elif defined RELEASE

char ver[] = "Release " STR(RELEASE);
char sshver[] = "PuTTY-Release-" STR(RELEASE);

#elif defined PRERELEASE

char ver[] = "Pre-release " STR(PRERELEASE) ":r" STR(SVN_REV);
char sshver[] = "PuTTY-Prerelease-" STR(PRERELEASE) ":r" STR(SVN_REV);

#elif defined SVN_REV

char ver[] = "Custom build r" STR(SVN_REV) ", " __DATE__ " " __TIME__;
char sshver[] = "PuTTY-Custom-r" STR(SVN_REV);

#else

char ver[] = "Unidentified build, " __DATE__ " " __TIME__;
char sshver[] = "PuTTY-Local: " __DATE__ " " __TIME__;

#endif

/*
 * SSH local version string MUST be under 40 characters. Here's a
 * compile time assertion to verify this.
 */
enum { vorpal_sword = 1 / (sizeof(sshver) <= 40) };
