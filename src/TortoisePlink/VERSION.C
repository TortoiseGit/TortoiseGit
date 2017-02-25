/*
 * PuTTY version numbering
 */

/*
 * The difficult part of deciding what goes in these version strings
 * is done in Buildscr, and then written into version.h. All we have
 * to do here is to drop it into variables of the right names.
 */

#ifdef SOURCE_COMMIT
#include "empty.h"
#endif

#include "version.h"

const char ver[] = TEXTVER;
const char sshver[] = SSHVER;
const char commitid[] = SOURCE_COMMIT;

/*
 * SSH local version string MUST be under 40 characters. Here's a
 * compile time assertion to verify this.
 */
enum { vorpal_sword = 1 / (sizeof(sshver) <= 40) };
