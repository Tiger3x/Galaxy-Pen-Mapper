#pragma once

// SipHash-2-4 for exactly eight input bytes (the IRP identity). Only the keyed
// token leaves the kernel; no pointer or session key is exported to CSV.
static __inline unsigned long long GalaxyDiagRotate(unsigned long long x, unsigned int n)
{ return (x << n) | (x >> (64 - n)); }
#define GALAXY_DIAG_ROUND() do { \
    v0 += v1; v1 = GalaxyDiagRotate(v1,13); v1 ^= v0; v0 = GalaxyDiagRotate(v0,32); \
    v2 += v3; v3 = GalaxyDiagRotate(v3,16); v3 ^= v2; \
    v0 += v3; v3 = GalaxyDiagRotate(v3,21); v3 ^= v0; \
    v2 += v1; v1 = GalaxyDiagRotate(v1,17); v1 ^= v2; v2 = GalaxyDiagRotate(v2,32); \
} while (0)
static __inline unsigned long long GalaxyDiagToken(unsigned long long identity,
    unsigned long long key0, unsigned long long key1)
{
    unsigned long long v0 = 0x736f6d6570736575ULL ^ key0;
    unsigned long long v1 = 0x646f72616e646f6dULL ^ key1;
    unsigned long long v2 = 0x6c7967656e657261ULL ^ key0;
    unsigned long long v3 = 0x7465646279746573ULL ^ key1;
    const unsigned long long tail = 8ULL << 56;
    v3 ^= identity; GALAXY_DIAG_ROUND(); GALAXY_DIAG_ROUND(); v0 ^= identity;
    v3 ^= tail; GALAXY_DIAG_ROUND(); GALAXY_DIAG_ROUND(); v0 ^= tail;
    v2 ^= 0xff; GALAXY_DIAG_ROUND(); GALAXY_DIAG_ROUND(); GALAXY_DIAG_ROUND(); GALAXY_DIAG_ROUND();
    return v0 ^ v1 ^ v2 ^ v3;
}
#undef GALAXY_DIAG_ROUND
