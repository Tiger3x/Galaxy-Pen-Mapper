#pragma once
#include "GalaxyPenMapperProtocol.h"

static __inline void GalaxyPenResetContactStats(GALAXY_PEN_PROBE_STATS* stats)
{
    stats->ContactReports = stats->SaturatedContactReports = 0;
    stats->ContactRawMin = stats->ContactRawMax = 0;
}
static __inline void GalaxyPenRecordContact(GALAXY_PEN_PROBE_STATS* stats,
    unsigned int flags, unsigned int raw)
{
    if (flags != 0x2Cu || raw > 4095u) return;
    if (!stats->ContactReports) stats->ContactRawMin = stats->ContactRawMax = raw;
    if (raw < stats->ContactRawMin) stats->ContactRawMin = raw;
    if (raw > stats->ContactRawMax) stats->ContactRawMax = raw;
    if (stats->ContactReports != 0xFFFFFFFFu) {
        ++stats->ContactReports;
        if (raw == 4095u) ++stats->SaturatedContactReports;
    }
}
