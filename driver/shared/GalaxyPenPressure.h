#pragma once
#include "GalaxyPenPressureLimits.h"

// Artificial baseline applies only to verified contact, never hover. It does
// not estimate force when the input is saturated. Preserve a representable
// floor when the HID value is reduced to Windows' 0..1024 scale.
static __inline unsigned int GalaxyPenTransformPressure(unsigned int raw, unsigned int sensitivityPermille,
    unsigned int floorPercent)
{
    unsigned int pressure;
    if (raw > 4095u) raw = 4095u;
    if (sensitivityPermille < GALAXY_PEN_MIN_SENSITIVITY) sensitivityPermille = GALAXY_PEN_MIN_SENSITIVITY;
    if (sensitivityPermille > GALAXY_PEN_MAX_SENSITIVITY) sensitivityPermille = GALAXY_PEN_MAX_SENSITIVITY;
    if (floorPercent < 1u) floorPercent = 1u;
    if (floorPercent > 25u) floorPercent = 25u;
    pressure = (4095u * floorPercent + 99u) / 100u;
    pressure += ((4095u - raw) * sensitivityPermille) / 1000u;
    if (pressure > 4095u) pressure = 4095u;
    return pressure;
}
