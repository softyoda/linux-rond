// SPDX-License-Identifier: GPL-2.0-or-later
#include "tapstate.h"
#include <input.h>
#include <input_event.h>
#include <input_event_spy.h>
#include <plugin.h>
#include <workspace.h>
#include <QTimer>
#include <QDebug>

class TouchWakeup final : public KWin::Plugin, public KWin::InputEventSpy {
public:
    TouchWakeup() { KWin::input()->installInputEventSpy(this); }
    void touchDown(KWin::TouchDownEvent *event) override {
        state.down(event->id, KWin::workspace()->dpmsState() != KWin::Workspace::DpmsState::On);
    }
    void touchUp(KWin::TouchUpEvent *event) override {
        const bool off = KWin::workspace()->dpmsState() != KWin::Workspace::DpmsState::On;
        if (state.up(event->id, off)) {
            // Let the DPMS filter consume the release before waking the display.
            QTimer::singleShot(0, this, [] {
                if (KWin::workspace()->dpmsState() != KWin::Workspace::DpmsState::On) {
                    KWin::workspace()->requestDpmsState(KWin::Workspace::DpmsState::On);
                    qInfo("Linux Rond: display woken by a single touch");
                }
            });
        }
    }
private:
    TapWakeState state;
};

class TouchWakeupFactory final : public KWin::PluginFactory {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginFactory_iid FILE "metadata.json")
    Q_INTERFACES(KWin::PluginFactory)
public:
    std::unique_ptr<KWin::Plugin> create() const override {
        return std::make_unique<TouchWakeup>();
    }
};
#include "touchwakeup.moc"
