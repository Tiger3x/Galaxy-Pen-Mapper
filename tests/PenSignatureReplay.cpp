#include "../src/input/PenSignature.h"

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (size_t index = 0; index < line.size(); ++index) {
        const char ch = line[index];
        if (ch == '"') {
            if (quoted && index + 1 < line.size() && line[index + 1] == '"') {
                field += '"';
                ++index;
            } else {
                quoted = !quoted;
            }
        } else if (ch == ',' && !quoted) {
            fields.push_back(field);
            field.clear();
        } else {
            field += ch;
        }
    }
    fields.push_back(field);
    return fields;
}

std::chrono::milliseconds timeOfDay(const std::string& timestamp) {
    if (timestamp.size() < 19) return std::chrono::milliseconds(0);
    const int hour = std::stoi(timestamp.substr(11, 2));
    const int minute = std::stoi(timestamp.substr(14, 2));
    const int second = std::stoi(timestamp.substr(17, 2));
    const int millis = timestamp.size() >= 23 ? std::stoi(timestamp.substr(20, 3)) : 0;
    return std::chrono::milliseconds(((hour * 60 + minute) * 60 + second) * 1000 + millis);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: PenSignatureReplay capture.csv\n";
        return 1;
    }
    std::ifstream input(argv[1]);
    if (!input) return 2;
    std::string line;
    if (!std::getline(input, line)) return 3;
    const auto header = parseCsvRow(line);
    std::unordered_map<std::string, size_t> columns;
    for (size_t index = 0; index < header.size(); ++index) columns[header[index]] = index;
    for (const char* required : {"timestamp", "event", "device", "vid", "pid", "usage_page", "usage", "raw_hex"}) {
        if (!columns.contains(required)) return 4;
    }

    PenSignatureTracker tracker;
    std::array<unsigned, 3> counts{};
    unsigned wcomReports = 0;
    std::string firstSPen;
    std::string firstPW500;
    while (std::getline(input, line)) {
        auto fields = parseCsvRow(line);
        if (fields.size() < header.size()) fields.resize(header.size()); // Legacy rows vary in trailing empty cells.
        if (fields.size() > header.size()) {
            bool extraFieldsEmpty = true;
            for (size_t index = header.size(); index < fields.size(); ++index) {
                if (!fields[index].empty()) extraFieldsEmpty = false;
            }
            if (extraFieldsEmpty) fields.resize(header.size());
        }
        if (fields.size() != header.size() || fields[columns["event"]] != "RAW_HID" ||
            fields[columns["device"]].find("WCOM016C") == std::string::npos) continue;
        RawHidReport report;
        report.deviceName = L"WCOM016C";
        report.vendorId = std::stoul(fields[columns["vid"]]);
        report.productId = std::stoul(fields[columns["pid"]]);
        report.usagePage = std::stoul(fields[columns["usage_page"]]);
        report.usage = std::stoul(fields[columns["usage"]]);
        report.bytes = fields[columns["raw_hex"]];
        const auto stamp = fields[columns["timestamp"]];
        const PenSignatureTracker::TimePoint now(timeOfDay(stamp));
        if (!tracker.observe(report, now)) continue;
        ++wcomReports;
        const auto signature = tracker.signature();
        ++counts[static_cast<size_t>(signature)];
        if (signature == PenSignature::SPenPattern && firstSPen.empty()) firstSPen = stamp;
        if (signature == PenSignature::PW500Candidate && firstPW500.empty()) firstPW500 = stamp;
    }
    std::cout << "WCOM=" << wcomReports << " inconclusive=" << counts[0]
              << " s_pen=" << counts[1] << " pw500_candidate=" << counts[2]
              << " first_s_pen=" << firstSPen << " first_pw500=" << firstPW500 << '\n';
    return 0;
}
