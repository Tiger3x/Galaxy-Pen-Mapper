#pragma once

#include <stddef.h>

// Candidate PW500 COL01 correction. Call only when manual PW500 mode is enabled
// and the live report path has been independently verified. Never synthesize
// contact from a hover report: button actions can legitimately suspend the tip.
static inline int GalaxyPenNormalizePw500ToolFlags(unsigned char* report, size_t length)
{
    if (report == NULL || length != 15 || report[0] != 0x02) return 0;

    switch (report[1]) {
    case 0x28: // InRange + Invert -> InRange (normal pen hover)
        report[1] = 0x20;
        return 1;
    case 0x2C: // InRange + Invert + Eraser -> InRange + Tip
        report[1] = 0x21;
        return 1;
    default:
        return 0;
    }
}
