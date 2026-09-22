#pragma once

#include "RawHidCapture.h"

#include <chrono>
#include <optional>

enum class PenSignature {
    Inconclusive,
    SPenPattern,
    PW500Candidate,
};

class PenSignatureTracker {
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    // Returns true only for a recognized WCOM screen report.
    bool observe(const RawHidReport& report, TimePoint now);
    void tick(TimePoint now);
    void reset();

    PenSignature signature() const { return current; }
    std::optional<unsigned> rawPressure() const { return latestPressure; }

private:
    PenSignature current = PenSignature::Inconclusive;
    std::optional<unsigned> latestPressure;
    std::optional<TimePoint> lastReport;
    std::optional<TimePoint> pw500PatternStart;
    bool sawSPenPattern = false;
    unsigned sPenSamples = 0;
    unsigned pw500ContactSamples = 0;
    unsigned pw500SaturatedSamples = 0;
};

const char* penSignatureCode(PenSignature signature);
