// SPDX-License-Identifier: GPL-2.0-or-later
#include "inputcoordinates.h"
#include <cstdlib>

using namespace LinuxRond;
static void check(bool value) { if (!value) std::abort(); }

int main()
{
    const QPointF initial(100, 150);
    const QPointF source(180, 230);
    check(touchSeatPosition(source, initial, initial) == source);
    const QPointF moved(130, 190);
    const QPointF delivered = touchSeatPosition(source, moved, initial);
    check(delivered - initial == source - moved);
    check(touchSeatPosition(QPointF(200, 300), moved, initial) - initial == QPointF(70, 110));
    check(touchSeatPosition(source + moved - initial, moved, initial) - initial == source - initial);
    const QPointF fractionalInitial(10.25, -3.75), fractionalCurrent(-11.5, 5.125);
    check(touchSeatPosition(QPointF(123.75, 88.5), fractionalCurrent, fractionalInitial)
          - fractionalInitial == QPointF(135.25, 83.375));
    const QPointF otherInitial(-200, 70), otherCurrent(-120, 40);
    check(touchSeatPosition(QPointF(-90, 100), otherCurrent, otherInitial) - otherInitial == QPointF(30, 60));
}
