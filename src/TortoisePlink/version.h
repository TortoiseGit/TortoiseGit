/*
 * This header file provides the various versioning-related #defines
 * for a particular PuTTY build.
 *
 * When my automated build system does a full build, Buildscr
 * completely overwrites this file with information derived from the
 * circumstances and type of that build. The information _here_ is
 * default stuff used for local development runs of 'make'.
 */

#define RELEASE 0.68
#define TEXTVER "Release 0.68"
#define SSHVER "TortoiseGitPlink-Release-0.68"
#define BINARY_VERSION 0,68,0,0
#define SOURCE_COMMIT "23fbc4f56b04ca5d387c16720caa05ddf2d63e2f"
// don't forget to update windows\TortoisePlink.rc

