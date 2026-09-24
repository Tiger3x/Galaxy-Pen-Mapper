#include <windows.h>
#include "GalaxyPenClientState.h"
#include "GalaxyPenPressureStats.h"
#include <cstdlib>
#include <iostream>

void Require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
}
int main() {
    static_assert(sizeof(GALAXY_PEN_CONFIG) == 24);
    static_assert(sizeof(GALAXY_PEN_PROBE_STATS) == 96);
    GALAXY_PEN_PROBE_STATS stats{};
    stats.Version = GALAXY_PEN_PROBE_PROTOCOL_VERSION;
    stats.Capabilities = GALAXY_PEN_CAP_OBSERVE;
    stats.AttachedDevices = 1;
    stats.ReadyForCorrection = 1;
    Require(GalaxyPenValidStats(&stats, sizeof(stats)) && !GalaxyPenCanEnable(&stats, sizeof(stats)), "Probe can activate");
    stats.Capabilities |= GALAXY_PEN_CAP_TOOL;
    Require(GalaxyPenCanEnable(&stats, sizeof(stats)), "Ready corrector blocked");
    stats.ReadyForCorrection = 0;
    Require(!GalaxyPenCanEnable(&stats, sizeof(stats)), "No-live-read gate bypassed");
    stats.ReadyForCorrection = 1; stats.AttachedDevices = 2;
    Require(!GalaxyPenCanEnable(&stats, sizeof(stats)), "Multiple devices allowed");
    stats.AttachedDevices = 1; --stats.Version;
    Require(!GalaxyPenValidStats(&stats, sizeof(stats)), "Old protocol accepted");
    stats.Version = GALAXY_PEN_PROBE_PROTOCOL_VERSION;
    Require(!GalaxyPenValidStats(&stats, sizeof(stats) - 1), "Partial status accepted");
    Require(!GalaxyPenValidStats(nullptr, sizeof(stats)), "Null status accepted");
    GALAXY_PEN_CONFIG config{GALAXY_PEN_PROTOCOL_VERSION, 0, 4500, 0, 0, 5};
    Require(GalaxyPenValidConfig(&config, sizeof(config)), "Valid configuration rejected");
    config.PressureEnabled = 2;
    Require(!GalaxyPenValidConfig(&config, sizeof(config)), "Invalid pressure mode accepted");
    config.PressureEnabled = 0; config.StateFlags = 0x80;
    Require(!GalaxyPenValidConfig(&config, sizeof(config)), "Unknown flags accepted");
    config.StateFlags = 0; config.ContactFloorPercent = 0;
    Require(!GalaxyPenValidConfig(&config, sizeof(config)), "Zero floor accepted");
    config.ContactFloorPercent = 26;
    Require(!GalaxyPenValidConfig(&config, sizeof(config)), "Excessive floor accepted");
    config.ContactFloorPercent = 5;
    config.SensitivityPermille = GALAXY_PEN_MAX_SENSITIVITY;
    Require(GalaxyPenValidConfig(&config, sizeof(config)), "Extended gain rejected");
    ++config.SensitivityPermille;
    Require(!GalaxyPenValidConfig(&config, sizeof(config)), "Out-of-range gain accepted");
    config.SensitivityPermille = GALAXY_PEN_MIN_SENSITIVITY - 1;
    Require(!GalaxyPenValidConfig(&config, sizeof(config)), "Below-minimum gain accepted");
    config.SensitivityPermille = GALAXY_PEN_DEFAULT_SENSITIVITY;
    config.Version = 3;
    Require(!GalaxyPenValidConfig(&config, sizeof(config)), "Old 800-percent protocol accepted");
    config.Version = GALAXY_PEN_PROTOCOL_VERSION;
    Require(!GalaxyPenValidConfig(&config, 20), "Old configuration layout accepted");
    GalaxyPenResetContactStats(&stats);
    GalaxyPenRecordContact(&stats, 0x28, 0);
    GalaxyPenRecordContact(&stats, 0x2C, 65535);
    Require(stats.ContactReports == 0, "Hover or invalid pressure counted as contact");
    GalaxyPenRecordContact(&stats, 0x2C, 4095);
    GalaxyPenRecordContact(&stats, 0x2C, 4095);
    Require(stats.ContactRawMin == 4095 && stats.ContactRawMax == 4095 && stats.SaturatedContactReports == 2,
        "Saturated signal presented as variation");
    GalaxyPenRecordContact(&stats, 0x2C, 4000);
    Require(stats.ContactReports == 3 && stats.ContactRawMin == 4000 && stats.ContactRawMax == 4095,
        "Contact extrema lost");
    stats.ContactReports = stats.SaturatedContactReports = 0xFFFFFFFFu;
    GalaxyPenRecordContact(&stats, 0x2C, 4095);
    Require(stats.ContactReports == 0xFFFFFFFFu && stats.SaturatedContactReports == 0xFFFFFFFFu, "Counters wrapped");
    GalaxyPenResetContactStats(&stats);
    Require(stats.ContactReports == 0 && stats.ContactRawMax == 0, "Old contact telemetry survived reset");
    std::cout << "Driver client: diagnostics, activation gates and protocol validation passed.\n";
}
