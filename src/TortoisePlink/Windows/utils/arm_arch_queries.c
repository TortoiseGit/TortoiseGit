/*
 * Windows implementation of the OS query functions that detect Arm
 * architecture extensions.
 */

#include "putty.h"
#include "ssh.h"

#if !(defined _M_ARM || defined _M_ARM64)
/*
 * For non-Arm, stub out these functions. This module shouldn't be
 * _called_ in that situation anyway, but it will still be compiled
 * (because that's easier than getting CMake to identify the
 * architecture in all cases).
 */
#define IsProcessorFeaturePresent(...) false
#endif

/* This feature id is documented in IsProcessorFeaturePresent as of
 * 2026-05-10, but is not in the version of MSVC I'm building with. I
 * expect it will show up in a later version. */
#ifndef PF_ARM_SHA3_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SHA3_INSTRUCTIONS_AVAILABLE 64
#endif

bool platform_aes_neon_available(void)
{
    return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
}

bool platform_pmull_neon_available(void)
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
    /*
     * Arm architecture extension nomenclature considers SHA-512 to be
     * part of the same extension as SHA-3, even though it's part of
     * the SHA-2 standard from NIST's point of view (and much more
     * closely related to SHA-256 in design than it is to SHA-3).
     */
    return IsProcessorFeaturePresent(PF_ARM_SHA3_INSTRUCTIONS_AVAILABLE);
}

bool platform_dit_available(void)
{
    /* As of 2024-12-17, as far as I can tell from docs.microsoft.com,
     * Windows on Arm does not yet provide a PF_ARM_V8_* flag for the
     * DIT bit in PSTATE. */
    return false;
}
