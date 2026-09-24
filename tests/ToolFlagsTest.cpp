#include "GalaxyPenToolFlags.h"

#include <array>
#include <cstddef>
#include <cstdlib>

namespace {

using Report = std::array<unsigned char, 15>;

void Require(bool condition)
{
    if (!condition) std::exit(EXIT_FAILURE);
}

Report MakeReport(unsigned char id, unsigned char flags)
{
    Report report{};
    report[0] = id;
    report[1] = flags;
    for (std::size_t i = 2; i < report.size(); ++i) {
        report[i] = static_cast<unsigned char>(i * 17);
    }
    return report;
}

void ExpectOnlyFlagChanged(unsigned char original, unsigned char expected)
{
    Report report = MakeReport(0x02, original);
    const Report before = report;
    Require(GalaxyPenNormalizePw500ToolFlags(report.data(), report.size()) == 1);
    Require(report[1] == expected);
    for (std::size_t i = 0; i < report.size(); ++i) {
        if (i != 1) Require(report[i] == before[i]);
    }
}

void ExpectUnchanged(unsigned char id, unsigned char flags, std::size_t length)
{
    Report report = MakeReport(id, flags);
    const Report before = report;
    Require(GalaxyPenNormalizePw500ToolFlags(report.data(), length) == 0);
    Require(report == before);
}

} // namespace

int main()
{
    ExpectOnlyFlagChanged(0x28, 0x20);
    ExpectOnlyFlagChanged(0x2C, 0x21);
    ExpectUnchanged(0x02, 0x20, 15); // Native pen hover.
    ExpectUnchanged(0x02, 0x21, 15); // Native pen contact.
    ExpectUnchanged(0x02, 0x00, 15); // Out of range.
    ExpectUnchanged(0x02, 0x2C, 14); // Truncated report.
    ExpectUnchanged(0x1A, 0x2C, 15); // Wrong collection/report ID.
    Require(GalaxyPenNormalizePw500ToolFlags(nullptr, 15) == 0);
}
