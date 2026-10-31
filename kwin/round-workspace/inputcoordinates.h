// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QPointF>

namespace LinuxRond {
// Seat keeps the origin captured at touch-down; compensate when the buffer moves.
inline QPointF touchSeatPosition(QPointF source, QPointF currentOrigin, QPointF initialOrigin)
{
    return source - currentOrigin + initialOrigin;
}
}
