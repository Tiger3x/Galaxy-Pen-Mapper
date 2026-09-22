#include "PenSignature.h"

#include <array>
#include <sstream>

namespace {

constexpr auto kIdleReset = std::chrono::milliseconds(1500);
constexpr auto kPW500Evidence = std::chrono::milliseconds(1000);
constexpr unsigned kSPenSamples = 20;
constexpr unsigned kPW500ContactSamples = 20;

std::optional<std::array<unsigned, 8>> firstReportBytes(const std::string& hex) {
    std::istringstream stream(hex);
    std::array<unsigned, 8> bytes{};
    for (auto& byte : bytes) {
        if (!(stream >> std::hex >> byte) || byte > 0xFF) return std::nullopt;
    }
    return bytes;
}

} // namespace

bool PenSignatureTracker::observe(const RawHidReport& report, TimePoint now) {
    if (report.vendorId != 11551 || report.productId != 337 ||
        report.usagePage != 0x0D || report.usage != 0x02 ||
        report.deviceName.find(L"WCOM016C") == std::wstring::npos) return false;

    const auto bytes = firstReportBytes(report.bytes);
    if (!bytes || (*bytes)[0] != 0x02) return false;

    tick(now);
    lastReport = now;
    const unsigned mode = (*bytes)[1];
    latestPressure = (*bytes)[6] | ((*bytes)[7] << 8);

    if (mode == 0x20 || mode == 0x21) {
        if (current == PenSignature::PW500Candidate) current = PenSignature::Inconclusive;
        sawSPenPattern = true;
        ++sPenSamples;
        pw500PatternStart.reset();
        pw500ContactSamples = 0;
        pw500SaturatedSamples = 0;
        if (sPenSamples >= kSPenSamples) current = PenSignature::SPenPattern;
    } else if (mode == 0x28 || mode == 0x2C) {
        if (!pw500PatternStart) pw500PatternStart = now;
        if (mode == 0x2C) {
            ++pw500ContactSamples;
            if (*latestPressure >= 4000) ++pw500SaturatedSamples;
        }
        // The S Pen also emitted short 0x28/0x2C bursts in the reference capture.
        // Never promote a mixed epoch to PW500 based on that shared mode.
        if (sawSPenPattern) {
            if (now - *pw500PatternStart >= kPW500Evidence) current = PenSignature::Inconclusive;
        } else if (now - *pw500PatternStart >= kPW500Evidence &&
                   pw500ContactSamples >= kPW500ContactSamples &&
                   pw500SaturatedSamples * 2 >= pw500ContactSamples) {
            current = PenSignature::PW500Candidate;
        }
    }
    return true;
}

void PenSignatureTracker::tick(TimePoint now) {
    if (lastReport && now - *lastReport >= kIdleReset) reset();
}

void PenSignatureTracker::reset() {
    current = PenSignature::Inconclusive;
    latestPressure.reset();
    lastReport.reset();
    pw500PatternStart.reset();
    sawSPenPattern = false;
    sPenSamples = 0;
    pw500ContactSamples = 0;
    pw500SaturatedSamples = 0;
}

const char* penSignatureCode(PenSignature signature) {
    switch (signature) {
        case PenSignature::SPenPattern: return "S_PEN_PATTERN";
        case PenSignature::PW500Candidate: return "PW500_CANDIDATE";
        default: return "INCONCLUSIVE";
    }
}
