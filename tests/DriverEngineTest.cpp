#include "GalaxyPenEngine.h"
#include <array>
#include <cstdlib>
#include <iostream>

using Report = std::array<unsigned char, 15>;
void Require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
}
Report MakeReport(unsigned char flag, unsigned pressure = 4000) {
    Report result{0x02, flag, 0x98, 0x3A, 0x40, 0x1F, 0, 0, 0x31, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    result[6] = static_cast<unsigned char>(pressure);
    result[7] = static_cast<unsigned char>(pressure >> 8);
    return result;
}
int main() {
    GALAXY_PEN_ENGINE engine{1, 0, 0, 0, 0, 4500, 5};
    auto contact = MakeReport(0x2C);
    const auto original = contact;
    Require(!GalaxyPenEngineProcess(&engine, contact.data(), contact.size(), engine.Generation, 1) && contact == original,
            "Disabled mode changed input");
    const auto oldGeneration = engine.Generation;
    GalaxyPenEngineConfigure(&engine, 1, 0, 4500, 5, 100);
    Require(!GalaxyPenEngineProcess(&engine, contact.data(), contact.size(), engine.Generation, 101) && contact == original,
            "Enable during contact must wait for lift");
    auto unknown = MakeReport(0xFF);
    GalaxyPenEngineProcess(&engine, unknown.data(), unknown.size(), engine.Generation, 102);
    Require(engine.WaitForLift == 1, "Unknown flags must not arm correction");
    auto invalidHover = MakeReport(0x28, 65535);
    Require(!GalaxyPenEngineProcess(&engine, invalidHover.data(), invalidHover.size(), engine.Generation, 102) &&
        engine.WaitForLift, "Invalid hover must not arm correction");
    auto hover = MakeReport(0x28, 0);
    Require(!GalaxyPenEngineProcess(&engine, hover.data(), hover.size(), oldGeneration, 103), "Old request changed");
    Require(engine.WaitForLift == 1, "Old request cleared lift gate");
    Require(GalaxyPenEngineProcess(&engine, hover.data(), hover.size(), engine.Generation, 104) && hover[1] == 0x20,
            "Hover not normalized or contact invented");
    Require(GalaxyPenEngineProcess(&engine, contact.data(), contact.size(), engine.Generation, 105) && contact[1] == 0x21,
            "Eraser contact not mapped to tip");
    for (size_t i = 0; i < contact.size(); ++i)
        if (i != 1) Require(contact[i] == original[i], "Flags-only mode changed another field");

    const auto flagsGeneration = engine.Generation;
    GalaxyPenEngineConfigure(&engine, 1, 1, 4500, 5, 200);
    Require(engine.WaitForLift == 1 && engine.Generation != flagsGeneration, "Pressure change must rearm");
    hover = MakeReport(0x28, 0);
    GalaxyPenEngineProcess(&engine, hover.data(), hover.size(), engine.Generation, 201);
    contact = original;
    Require(GalaxyPenEngineProcess(&engine, contact.data(), contact.size(), engine.Generation, 202) != 0, "Pressure mode inactive");
    Require(contact[1] == 0x21 && (contact[6] | (unsigned(contact[7]) << 8)) == 632, "Wrong pressure result");
    for (size_t i = 0; i < contact.size(); ++i)
        if (i != 1 && i != 6 && i != 7) Require(contact[i] == original[i], "Position or other fields changed");

    for (unsigned flag = 0; flag < 256; ++flag) {
        if (flag == 0x28 || flag == 0x2C) continue;
        auto report = MakeReport(static_cast<unsigned char>(flag));
        const auto before = report;
        GalaxyPenEngineProcess(&engine, report.data(), report.size(), engine.Generation, 203);
        Require(report == before, "Unrecognized/native state was modified");
    }
    auto invalid = MakeReport(0x2C, 5000);
    auto before = invalid;
    Require(!GalaxyPenEngineProcess(&engine, invalid.data(), invalid.size(), engine.Generation, 204) && invalid == before,
            "Invalid pressure report was modified");
    invalid = original; invalid[0] = 0x1A; before = invalid;
    Require(!GalaxyPenEngineProcess(&engine, invalid.data(), invalid.size(), engine.Generation, 205) && invalid == before,
            "Other collection modified");
    Require(!GalaxyPenEngineProcess(&engine, invalid.data(), 14, engine.Generation, 206), "Short report accepted");
    Require(!GalaxyPenEngineProcess(&engine, nullptr, 15, engine.Generation, 207), "Null report accepted");

    // Heartbeat updates must not reset the lift gate or the request generation.
    const auto activeGeneration = engine.Generation;
    GalaxyPenEngineConfigure(&engine, 1, 1, 4500, 5, 1000);
    Require(engine.Generation == activeGeneration && !engine.WaitForLift, "Heartbeat rearmed mode");
    contact = original;
    Require(!GalaxyPenEngineProcess(&engine, contact.data(), contact.size(), activeGeneration,
            1000 + GALAXY_PEN_LEASE_100NS) && contact == original && !engine.Enabled,
            "Expired lease changed report or stayed enabled");
    GalaxyPenEngineConfigure(&engine, 1, 0, 4500, 5, 2000);
    const auto preDisable = engine.Generation;
    GalaxyPenEngineDisable(&engine); // Same path used by close, suspend and removal.
    contact = original;
    Require(!GalaxyPenEngineProcess(&engine, contact.data(), contact.size(), preDisable, 2001) && contact == original,
            "Completion after disable changed input");
    GalaxyPenEngineConfigure(&engine, 1, 0, 4500, 5, 3000);
    GalaxyPenEngineExpire(&engine, 2999);
    Require(!engine.Enabled, "Clock rollback must fail open");

    GalaxyPenEngineConfigure(&engine, 1, 1, 4500, 5, 4000);
    hover = MakeReport(0x28, 0);
    GalaxyPenEngineProcess(&engine, hover.data(), hover.size(), engine.Generation, 4001);
    Require(hover[6] == 0 && hover[7] == 0 && hover[1] == 0x20, "Artificial pressure leaked into hover");
    contact = MakeReport(0x2C, 4095);
    GalaxyPenEngineProcess(&engine, contact.data(), contact.size(), engine.Generation, 4002);
    Require((contact[6] | (unsigned(contact[7]) << 8)) == 205, "Saturated contact still rounds to zero");
    const auto beforeFloorChange = engine.Generation;
    GalaxyPenEngineConfigure(&engine, 1, 1, 4500, 10, 4003);
    Require(engine.WaitForLift && engine.Generation != beforeFloorChange, "Floor change did not wait for lift");

    for (unsigned gain : {1000u, 4500u, 8000u, 12000u, 16000u, 32000u}) {
        GalaxyPenEngineConfigure(&engine, 1, 1, gain, 5, 5000);
        hover = MakeReport(0x28, 0);
        GalaxyPenEngineProcess(&engine, hover.data(), hover.size(), engine.Generation, 5001);
        Require(hover[6] == 0 && hover[7] == 0, "Extended gain leaked pressure into hover");
        contact = MakeReport(0x2C, 4083);
        const auto source = contact;
        GalaxyPenEngineProcess(&engine, contact.data(), contact.size(), engine.Generation, 5002);
        Require((contact[6] | (unsigned(contact[7]) << 8)) == GalaxyPenTransformPressure(4083, gain, 5), "Engine clamped extended gain");
        for (size_t i = 0; i < contact.size(); ++i)
            if (i != 1 && i != 6 && i != 7) Require(contact[i] == source[i], "Extended gain changed unrelated bytes");
        unsigned previous = 4095;
        for (unsigned raw = 0; raw <= 4095; ++raw) {
            const auto value = GalaxyPenTransformPressure(raw, gain, 5);
            Require(value >= 1 && value <= 4095 && value <= previous, "Pressure range/monotonicity failed");
            previous = value;
        }
    }
    std::cout << "Driver engine: lifecycle boundaries, flags, pressure, epoch and lease passed.\n";
}
