#include "../src/input/PenSignature.h"

#include <chrono>
#include <iostream>
#include <string>

namespace {

RawHidReport report(const char* mode, const char* pressure = "00 00") {
    RawHidReport result;
    result.deviceName = L"HID#WCOM016C&Col01";
    result.vendorId = 11551;
    result.productId = 337;
    result.usagePage = 0x0D;
    result.usage = 0x02;
    result.bytes = std::string("02 ") + mode + " 00 00 00 00 " + pressure;
    return result;
}

bool check(bool condition, const char* description) {
    if (!condition) std::cerr << description << '\n';
    return condition;
}

} // namespace

int main() {
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;
    const Clock::time_point start{};

    PenSignatureTracker tracker;
    for (int i = 0; i < 20; ++i) tracker.observe(report("20"), start + i * 5ms);
    if (!check(tracker.signature() == PenSignature::SPenPattern, "S Pen pattern was not recognized")) return 1;
    for (int i = 0; i < 60; ++i) tracker.observe(report("28"), start + 100ms + i * 5ms);
    if (!check(tracker.signature() == PenSignature::SPenPattern, "Short shared-mode burst changed identity")) return 2;
    tracker.tick(start + 2s);
    if (!check(tracker.signature() == PenSignature::Inconclusive, "Idle state was not cleared")) return 3;

    for (int i = 0; i < 220; ++i) tracker.observe(report("28"), start + 3s + i * 5ms);
    if (!check(tracker.signature() == PenSignature::Inconclusive, "Hover alone identified PW500")) return 4;
    for (int i = 0; i < 30; ++i) tracker.observe(report("2C", "FF 0F"), start + 4100ms + i * 5ms);
    if (!check(tracker.signature() == PenSignature::PW500Candidate &&
               tracker.rawPressure() == 4095u, "Sustained saturated PW500 pattern was not recognized")) return 5;

    RawHidReport other = report("20");
    other.deviceName = L"Other digitizer";
    if (!check(!tracker.observe(other, start + 4300ms) &&
               tracker.signature() == PenSignature::PW500Candidate,
               "Unrelated digitizer affected the result")) return 6;

    tracker.observe(report("20"), start + 4305ms);
    if (!check(tracker.signature() == PenSignature::Inconclusive,
               "Contradictory S Pen mode did not fail closed")) return 7;
    std::cout << "Pen signature states verified\n";
    return 0;
}
