#include "GalaxyPenEngine.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

std::vector<std::string> Csv(const std::string& line) {
    std::vector<std::string> result;
    std::string field;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '"') {
            if (quoted && i + 1 < line.size() && line[i + 1] == '"') { field += '"'; ++i; }
            else quoted = !quoted;
        } else if (line[i] == ',' && !quoted) { result.push_back(field); field.clear(); }
        else if (line[i] != '\r') field += line[i];
    }
    result.push_back(field);
    return result;
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: DriverCaptureReplay capture-directory\n"; return 2; }
    unsigned long long total = 0, changed = 0, files = 0;
    unsigned long long contacts = 0, saturated = 0;
    unsigned contactMin = 4095, contactMax = 0;
    for (const auto& entry : std::filesystem::directory_iterator(argv[1])) {
        if (!entry.is_regular_file() || entry.path().extension() != ".csv") continue;
        std::ifstream input(entry.path());
        std::string line;
        if (!std::getline(input, line)) continue;
        const auto header = Csv(line);
        std::unordered_map<std::string, size_t> columns;
        for (size_t i = 0; i < header.size(); ++i) columns[header[i]] = i;
        if (!columns.contains("event") || !columns.contains("device") || !columns.contains("raw_hex")) continue;
        GALAXY_PEN_ENGINE enabled{1, 0, 0, 0, 0, 4500, 5}, disabled{1, 0, 0, 0, 0, 4500, 5};
        unsigned long long now = 1, fileReports = 0;
        while (std::getline(input, line)) {
            auto fields = Csv(line);
            if (fields.size() < header.size()) fields.resize(header.size());
            std::string device = fields[columns["device"]];
            std::transform(device.begin(), device.end(), device.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            if (fields[columns["event"]] != "RAW_HID" || device.find("WCOM016C&COL01") == std::string::npos) continue;
            std::istringstream hex(fields[columns["raw_hex"]]);
            unsigned byte;
            std::vector<unsigned char> report;
            while (hex >> std::hex >> byte) {
                if (byte > 255) return 3;
                report.push_back(static_cast<unsigned char>(byte));
            }
            const auto before = report;
            if (report.size() == 15 && report[0] == 0x02 && report[1] == 0x2C) {
                const unsigned raw = unsigned(report[6]) | (unsigned(report[7]) << 8);
                if (raw <= 4095) {
                    ++contacts; if (raw == 4095) ++saturated;
                    contactMin = std::min(contactMin, raw); contactMax = std::max(contactMax, raw);
                }
            }
            auto off = report;
            if (GalaxyPenEngineProcess(&disabled, off.data(), off.size(), disabled.Generation, now) || off != before) return 4;
            GalaxyPenEngineConfigure(&enabled, 1, 1, 4500, 5, now);
            const bool modified = GalaxyPenEngineProcess(&enabled, report.data(), report.size(), enabled.Generation, now) != 0;
            ++now; ++total; ++fileReports;
            if (modified) {
                ++changed;
                if (report.size() != 15 || before[0] != 0x02 || (before[1] != 0x28 && before[1] != 0x2C)) return 5;
                if (report[1] != (before[1] == 0x28 ? 0x20 : 0x21)) return 6;
                for (size_t i = 0; i < report.size(); ++i)
                    if (i != 1 && i != 6 && i != 7 && report[i] != before[i]) return 7;
                const auto raw = unsigned(before[6]) | (unsigned(before[7]) << 8);
                const auto pressure = unsigned(report[6]) | (unsigned(report[7]) << 8);
                if (before[1] == 0x28 && pressure != raw) return 8;
                if (before[1] == 0x2C && pressure != GalaxyPenTransformPressure(raw, 4500, 5)) return 9;
            } else if (report != before) return 10;
        }
        if (fileReports) ++files;
    }
    std::cout << "Read-only replay: files=" << files << " WCOM_COL01=" << total << " normalized=" << changed
              << "; off-mode and unrelated bytes preserved. Timing tested separately.\n";
    std::cout << "PW500 contacts=" << contacts << " saturated=" << saturated << " raw range=" << contactMin << ".." << contactMax << '\n';
    return total && changed ? 0 : 1;
}
