/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: some common constants
 ******************************************************************************/
#pragma once
#include <limits>

inline constexpr double minf = -std::numeric_limits<double>::infinity();
inline constexpr double pinf = std::numeric_limits<double>::infinity();
inline constexpr double NaN = std::numeric_limits<double>::quiet_NaN();