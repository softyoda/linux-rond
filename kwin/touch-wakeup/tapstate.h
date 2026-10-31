// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <set>

// Wait until all fingers are lifted: the wake-up tap must not become a click
// on an application which never received the corresponding touch-down event.
class TapWakeState {
public:
    void down(int id, bool screenOff) {
        if (screenOff) {
            if (!armed) contacts.clear();
            armed = true;
            contacts.insert(id);
        } else if (armed) {
            contacts.clear();
            armed = false;
        }
    }
    bool up(int id, bool screenOff) {
        if (!armed) return false;
        if (!screenOff) { contacts.clear(); armed = false; return false; }
        if (!contacts.erase(id) || !contacts.empty()) return false;
        armed = false;
        return true;
    }
private:
    std::set<int> contacts;
    bool armed = false;
};
