/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: z values of a 3D graph with a log10 z axis
 ******************************************************************************/

#ifndef GRAPH3DLOGSCALE_H
#define GRAPH3DLOGSCALE_H

#include <cmath>

namespace Graph3DLogScale
{
// A value as the plot places it: on a log10 z axis its exponent, and a value <= 0 (no position there) at the lower
// limit zMin; then within zMin..zMax (log10 exponents on a log axis). Before, a value <= 0 kept its raw value, read as
// an exponent: 0 showed as 10^0 = 1.
inline double plotZ(double value, bool zLog, double zMin, double zMax)
{
    if (zLog)
        value = value > 0 ? std::log10(value) : zMin;
    if (value < zMin)
        return zMin;
    if (value > zMax)
        return zMax;
    return value;
}

// The same for a formula surface, which keeps its own range: only the log10 exponent, a value <= 0 at zMin
inline double logZ(double value, bool zLog, double zMin)
{
    if (!zLog)
        return value;
    return value > 0 ? std::log10(value) : zMin;
}
} // namespace Graph3DLogScale

#endif
