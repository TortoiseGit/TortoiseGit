/*
 * This header file provides the various versioning-related #defines
 * for a particular PuTTY build.
 *
 * When my automated build system does a full build, Buildscr
 * completely overwrites this file with information derived from the
 * circumstances and type of that build. The information _here_ is
 * default stuff used for local development runs of 'make'.
 */

#define RELEASE 0.70
#define TEXTVER "Release 0.70"
#define SSHVER "TortoiseGitPlink-Release-0.70"
#define BINARY_VERSION 0,70,0,0
#define SOURCE_COMMIT "3cd10509a51edf5a21cdc80aabf7e6a934522d47"
// don't forget to update windows\TortoisePlink.rc

