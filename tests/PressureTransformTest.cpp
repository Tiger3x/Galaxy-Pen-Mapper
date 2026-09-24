#include "../driver/shared/GalaxyPenPressure.h"

#include <cstdlib>
#include <iostream>

int main() {
    bool ok = true;
    const auto expect = [&ok](unsigned int raw, unsigned int gain, unsigned int wanted) {
        const unsigned int actual = GalaxyPenTransformPressure(raw, gain, 5);
        if (actual != wanted) {
            std::cerr << "raw=" << raw << " gain=" << gain << " expected=" << wanted
                      << " got=" << actual << '\n';
            ok = false;
        }
    };
    expect(4095, 4500, 205);
    expect(4000, 4500, 632);
    expect(3600, 4500, 2432);
    expect(3201, 4500, 4095);
    expect(3201, 1000, 1099);
    expect(3201, 8000, 4095);
    expect(5000, 4500, 205);
    expect(0, 4500, 4095);
    expect(4083, 8000, 301);
    expect(4083, 12000, 349);
    expect(4083, 16000, 397);
    expect(4083, 32000, 589);
    expect(4000, 32000, 3245);
    expect(3900, 32000, 4095);
    expect(4095, 32000, 205); // Gain must not raise the artificial floor.
    expect(4094, 0xffffffffu, 237); // Clamp before multiplying, avoid overflow.
    for (unsigned floor = 1; floor <= 25; ++floor) {
        const unsigned baseline = (4095 * floor + 99) / 100;
        for (unsigned gain = GALAXY_PEN_MIN_SENSITIVITY; gain <= GALAXY_PEN_MAX_SENSITIVITY; gain += 100) {
            unsigned previous = 4095;
            for (unsigned raw = 0; raw <= 4095; ++raw) {
                const auto value = GalaxyPenTransformPressure(raw, gain, floor);
                if (value < baseline || value > previous || value > 4095 || value * 1024 / 4095 == 0) ok = false;
                previous = value;
            }
            if (GalaxyPenTransformPressure(4095, gain, floor) != baseline) ok = false;
        }
    }
    if (GalaxyPenTransformPressure(4095, 0, 0) != 41 ||
        GalaxyPenTransformPressure(4095, 999999, 999999) != 1024) ok = false;
    if (ok) std::cout << "Pressure transform tests passed.\n";
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
