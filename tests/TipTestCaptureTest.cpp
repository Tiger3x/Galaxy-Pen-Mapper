#include "../src/ui/TipTestCapture.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
void Require(bool ok, const char* text) { if (!ok) { std::cerr << text << '\n'; std::exit(1); } }
int main() {
    TipTestCapture capture;
    Require(!capture.unsaved && capture.samples.empty(), "Initial capture not empty");
    Require(capture.Append({5, 582, 1, 20, 30, 16, 4, true, true, 800, -2, 3}), "Sample rejected");
    Require(capture.unsaved, "Unsaved guard absent");
    std::ostringstream out;
    capture.WriteCsv(out);
    Require(out.str().find("windows_pressure_0_1024") != std::string::npos, "Scale not documented");
    Require(out.str().find("5,582,1,20,30,16,4,1,1,800,-2,3\n") != std::string::npos, "CSV sample corrupted");
    Require(capture.unsaved, "Serialization clears dirty before disk success");
    while (capture.samples.size() < TipTestCapture::Limit) Require(capture.Append({}), "Capacity failure");
    Require(!capture.Append({}) && capture.samples.size() == TipTestCapture::Limit, "Unbounded recording");
    capture.Clear(); Require(capture.samples.empty() && !capture.unsaved, "Clear failed");
    std::cout << "Tip capture: CSV fields, dirty guard, limit and reset passed.\n";
}
