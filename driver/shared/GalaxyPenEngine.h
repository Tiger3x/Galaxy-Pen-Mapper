#pragma once

#include "GalaxyPenToolFlags.h"
#include "GalaxyPenPressure.h"

#define GALAXY_PEN_LEASE_100NS 25000000ULL

// Pure state machine shared by the driver and offline tests. Caller serializes
// access and supplies interrupt time in 100 ns units (not wall-clock time).
typedef struct _GALAXY_PEN_ENGINE {
    unsigned long long Generation;
    unsigned long long LastHeartbeat;
    unsigned int Enabled;
    unsigned int WaitForLift;
    unsigned int PressureEnabled;
    unsigned int Sensitivity;
    unsigned int ContactFloorPercent;
} GALAXY_PEN_ENGINE;

static __inline void GalaxyPenEngineDisable(GALAXY_PEN_ENGINE* engine)
{
    ++engine->Generation;
    engine->Enabled = 0;
    engine->WaitForLift = 0;
}

static __inline void GalaxyPenEngineExpire(GALAXY_PEN_ENGINE* engine, unsigned long long now)
{
    if (engine->Enabled && (now < engine->LastHeartbeat ||
        now - engine->LastHeartbeat >= GALAXY_PEN_LEASE_100NS)) {
        GalaxyPenEngineDisable(engine);
    }
}

static __inline void GalaxyPenEngineConfigure(GALAXY_PEN_ENGINE* engine,
    unsigned int enabled, unsigned int pressureEnabled, unsigned int sensitivity,
    unsigned int floorPercent, unsigned long long now)
{
    GalaxyPenEngineExpire(engine, now);
    if (!enabled) {
        if (engine->Enabled) GalaxyPenEngineDisable(engine);
    } else if (!engine->Enabled || engine->PressureEnabled != pressureEnabled ||
               engine->Sensitivity != sensitivity || engine->ContactFloorPercent != floorPercent) {
        ++engine->Generation;
        engine->Enabled = 1;
        engine->WaitForLift = 1;
    }
    engine->PressureEnabled = pressureEnabled;
    engine->Sensitivity = sensitivity;
    engine->ContactFloorPercent = floorPercent;
    engine->LastHeartbeat = now;
}

static __inline unsigned int GalaxyPenEngineProcess(GALAXY_PEN_ENGINE* engine,
    unsigned char* report, size_t length, unsigned long long requestGeneration,
    unsigned long long now)
{
    unsigned int raw;
    unsigned int pressure;
    unsigned char flags;
    GalaxyPenEngineExpire(engine, now);
    if (!engine->Enabled || requestGeneration != engine->Generation ||
        report == NULL || length != 15 || report[0] != 0x02) return 0;

    flags = report[1];
    raw = (unsigned int)report[6] | ((unsigned int)report[7] << 8);
    if (raw > 4095u) return 0;
    // Only descriptor-verified states qualify as a lift. Unknown reports must
    // not arm correction or create contact.
    if (flags == 0x00 || flags == 0x20 || flags == 0x28) engine->WaitForLift = 0;
    if (engine->WaitForLift || (flags != 0x28 && flags != 0x2C)) return 0;
    if (!GalaxyPenNormalizePw500ToolFlags(report, length)) return 0;
    if (flags == 0x2C && engine->PressureEnabled) {
        pressure = GalaxyPenTransformPressure(raw, engine->Sensitivity, engine->ContactFloorPercent);
        report[6] = (unsigned char)(pressure & 0xFFu);
        report[7] = (unsigned char)(pressure >> 8);
    }
    return 1;
}
