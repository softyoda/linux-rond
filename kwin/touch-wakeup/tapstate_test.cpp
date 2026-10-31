// SPDX-License-Identifier: GPL-2.0-or-later
#include "tapstate.h"
#include <cstdlib>
void check(bool v) { if (!v) std::abort(); }
int main() {
    TapWakeState s;
    check(!s.up(1,true));
    s.down(1,false); check(!s.up(1,false));
    s.down(1,true); check(s.up(1,true)); check(!s.up(1,true));
    s.down(1,true); s.down(2,true); check(!s.up(1,true)); check(s.up(2,true));
    s.down(3,true); check(!s.up(3,false));
    s.down(4,true); check(!s.up(5,true)); check(s.up(4,true));
    s.down(6,true); s.down(7,false); check(!s.up(6,true));
}
