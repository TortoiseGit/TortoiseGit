/*
 * Windows implementation of the OS query functions that detect Arm
 * architecture extensions.
 */

#include "putty.h"

#if !(defined _M_ARM || defined _M_ARM64)
/*
 * For non-Arm, stub out these functions. This module shouldn't be
 * _called_ in that situation anyway, but it will still be compiled
 * (because that's easier than getting CMake to identify the
 * architecture in all cases).
 */
#define IsProcessorFeaturePresent(...) false
#endif

bool platform_aes_neon_available(void)
{
    return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
}

bool platform_sha256_neon_available(void)
{
    return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
}

bool platform_sha1_neon_available(void)
{
    return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
}

bool platform_sha512_neon_available(void)
{
    /* As of 2020-12-24, as far as I can tell from docs.microsoft.com,
     * Windows on Arm does not yet provide a PF_ARM_V8_* flag for the
     * SHA-512 architecture extension. */
    return false;
}
