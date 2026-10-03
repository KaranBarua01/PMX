#include "analysis/TunerTap.h"
#include <array>
#include <cmath>

int main()
{
    pmx::analysis::TunerTap tap;
    tap.prepare(8);
    const float a[] { 0.1f, 0.2f, 0.3f, 0.4f };
    if (tap.push(a, 4) != 4) return 1;

    std::array<float, 3> first {};
    if (tap.pop(first.data(), static_cast<int>(first.size())) != 3) return 2;
    if (std::abs(first[0] - 0.1f) > 0.0001f || std::abs(first[2] - 0.3f) > 0.0001f) return 3;

    const float b[] { 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f };
    const auto accepted = tap.push(b, 8);
    if (accepted != 7) return 4; // one unread sample + seven new samples fills capacity
    if (tap.available() != 8) return 5;

    std::array<float, 8> all {};
    if (tap.pop(all.data(), 8) != 8) return 6;
    if (std::abs(all[0] - 0.4f) > 0.0001f || std::abs(all[7] - 1.1f) > 0.0001f) return 7;
    if (tap.available() != 0) return 8;
    return 0;
}
