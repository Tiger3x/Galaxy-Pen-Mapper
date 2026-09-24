#pragma once
#include <cstdint>
#include <ostream>
#include <vector>

struct TipTestSample {
    std::uint64_t elapsedMs;
    unsigned message, pointerId;
    int x, y;
    unsigned pointerFlags, penFlags;
    bool contact, pressureValid;
    unsigned pressure;
    int tiltX, tiltY;
};
class TipTestCapture {
public:
    static constexpr size_t Limit = 50000;
    std::vector<TipTestSample> samples;
    bool unsaved = false;
    bool Append(const TipTestSample& sample) {
        if (samples.size() >= Limit) return false;
        samples.push_back(sample);
        unsaved = true;
        return true;
    }
    void Clear() { samples.clear(); unsaved = false; }
    void WriteCsv(std::ostream& out) const {
        out << "elapsed_ms,message,pointer_id,x_client,y_client,pointer_flags,pen_flags,contact,pressure_valid,windows_pressure_0_1024,tilt_x,tilt_y\n";
        for (const auto& s : samples)
            out << s.elapsedMs << ',' << s.message << ',' << s.pointerId << ',' << s.x << ',' << s.y
                << ',' << s.pointerFlags << ',' << s.penFlags << ',' << s.contact << ',' << s.pressureValid
                << ',' << s.pressure << ',' << s.tiltX << ',' << s.tiltY << '\n';
    }
};
