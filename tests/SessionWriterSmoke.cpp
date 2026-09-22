#include "../src/capture/SessionWriter.h"

#include <fstream>
#include <iostream>
#include <string>
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

} // namespace

int main() {
    SessionWriter writer;
    if (!writer.start(L"Test", L"description, with comma")) return 1;
    writer.writeForegroundSample(false);
    RawHidReport report;
    report.deviceName = L"Digitizer";
    report.vendorId = 0x056A;
    report.productId = 0x1234;
    report.usagePage = 0x0D;
    report.usage = 2;
    report.bytes = "28 00 04";
    writer.writeRaw(report, false, RIM_INPUTSINK, "PW500_CANDIDATE", 4095);
    const auto path = writer.path();
    writer.stop();

    std::ifstream input(path);
    if (!input) return 2;
    std::string line;
    std::vector<std::vector<std::string>> rows;
    while (std::getline(input, line)) rows.push_back(parseCsvRow(line));
    if (rows.size() != 5) return 3;
    for (size_t index = 0; index < rows.size(); ++index) {
        if (rows[index].size() != 37) return 4;
        if (index != 0 && rows[index][1] != "description, with comma") return 4;
    }
    if (rows[2][3] != "FOREGROUND_SAMPLE" || rows[2][33] != "0") return 5;
    if (rows[3][3] != "RAW_HID" || rows[3][10] != "28 00 04" ||
        rows[3][33] != "0" || rows[3][34] != "1" ||
        rows[3][35] != "PW500_CANDIDATE" || rows[3][36] != "4095") return 6;
    if (rows[4][3] != "SESSION_STOP") return 7;
    std::cout << "CSV Raw HID/background fields verified\n";
    return 0;
}
