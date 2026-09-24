#pragma once
#include "GalaxyPenMapperProtocol.h"
#include <stddef.h>

static __inline int GalaxyPenValidStats(const GALAXY_PEN_PROBE_STATS* stats, size_t bytes)
{
    return stats != NULL && bytes == sizeof(*stats) &&
        stats->Version == GALAXY_PEN_PROBE_PROTOCOL_VERSION &&
        (stats->Capabilities & GALAXY_PEN_CAP_OBSERVE) != 0;
}

static __inline int GalaxyPenCanEnable(const GALAXY_PEN_PROBE_STATS* stats, size_t bytes)
{
    return GalaxyPenValidStats(stats, bytes) && stats->AttachedDevices == 1 &&
        stats->ReadyForCorrection == 1 && (stats->Capabilities & GALAXY_PEN_CAP_TOOL) != 0;
}

static __inline int GalaxyPenValidConfig(const GALAXY_PEN_CONFIG* config, size_t bytes)
{
    return config != NULL && bytes == sizeof(*config) &&
        config->Version == GALAXY_PEN_PROTOCOL_VERSION && config->Enabled <= 1 &&
        config->PressureEnabled <= 1 &&
        config->ContactFloorPercent >= 1 && config->ContactFloorPercent <= 25 &&
        (config->StateFlags & ~GALAXY_PEN_STATE_WAIT_FOR_LIFT) == 0 &&
        config->SensitivityPermille >= GALAXY_PEN_MIN_SENSITIVITY &&
        config->SensitivityPermille <= GALAXY_PEN_MAX_SENSITIVITY;
}
