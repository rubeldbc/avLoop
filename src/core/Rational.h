#pragma once

#include <cmath>
#include <cstdint>
#include <numeric>

namespace avloop {

// A rational number: frame rates (30000/1001) and time bases. Plain data, Qt-free.
struct Rational {
    int num = 0;
    int den = 1;

    constexpr bool isValid() const { return num > 0 && den > 0; }
    constexpr double value() const { return den ? double(num) / double(den) : 0.0; }
    Rational reduced() const {
        const int g = std::gcd(num, den);
        return g ? Rational{num / g, den / g} : *this;
    }
    friend constexpr bool operator==(const Rational& a, const Rational& b) {
        return std::int64_t(a.num) * b.den == std::int64_t(b.num) * a.den;
    }
};

} // namespace avloop
