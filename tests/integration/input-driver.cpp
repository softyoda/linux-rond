// SPDX-License-Identifier: GPL-2.0-or-later
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>
#include <libei.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <cstdio>

static void fail(const char *message) { qCritical("%s", message); std::exit(1); }
static uint64_t now() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static void requireIsolatedSession() {
    if (qEnvironmentVariable("LINUX_ROND_TEST_SESSION") != "1" ||
        !qEnvironmentVariable("WAYLAND_DISPLAY").startsWith("linux-rond-test-"))
        fail("Input injection is restricted to isolated Linux Rond test sessions.");
    const auto pid = QDBusConnection::sessionBus().interface()->servicePid("org.kde.KWin");
    if (!pid.isValid()) fail("Cannot identify test compositor.");
    QFile cmd(QString("/proc/%1/cmdline").arg(pid.value()));
    if (!cmd.open(QIODevice::ReadOnly)) fail("Cannot inspect test compositor.");
    const auto args = cmd.readAll().split('\0');
    if (!args.contains("--virtual") || !args.contains(qgetenv("WAYLAND_DISPLAY")))
        fail("Refusing to inject into a non-test compositor.");
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    requireIsolatedSession();
    auto args = app.arguments();
    if (args.size() < 4) fail("Usage: input-driver click|touch|move|drag|touch-drag|two-touch x y [x2 y2]");
    const auto action = args[1];
    const double x = args[2].toDouble(), y = args[3].toDouble();
    const double x2 = args.size()>4 ? args[4].toDouble() : x+60;
    const double y2 = args.size()>5 ? args[5].toDouble() : y+60;
    const bool touch = action.contains("touch");
    const bool keyboard = action=="key-overview";
    auto call = QDBusMessage::createMethodCall("org.kde.KWin", "/org/kde/KWin/EIS/RemoteDesktop", "org.kde.KWin.EIS.RemoteDesktop", "connectToEIS");
    call << 7;
    const auto reply = QDBusConnection::sessionBus().call(call);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) fail("Cannot connect to test EIS.");
    const auto fd = qvariant_cast<QDBusUnixFileDescriptor>(reply.arguments()[0]);
    if (!fd.isValid()) fail("Invalid EIS descriptor.");
    ei *context = ei_new_sender(nullptr);
    ei_configure_name(context, "linux-rond-test-input");
    if (ei_setup_backend_fd(context, dup(fd.fileDescriptor())) != 0) fail("EIS setup failed.");
    ei_device *device = nullptr;
    auto dispatch = [&] {
        QCoreApplication::processEvents();
        ei_dispatch(context);
        while (ei_event *event = ei_get_event(context)) {
            const auto type = ei_event_get_type(event);
            if (type == EI_EVENT_DISCONNECT) fail("Test EIS disconnected.");
            if (type == EI_EVENT_SEAT_ADDED) {
                ei_seat_bind_capabilities(ei_event_get_seat(event), EI_DEVICE_CAP_POINTER_ABSOLUTE,
                    EI_DEVICE_CAP_BUTTON, EI_DEVICE_CAP_TOUCH, EI_DEVICE_CAP_SCROLL, EI_DEVICE_CAP_KEYBOARD, nullptr);
            }
            if (type == EI_EVENT_DEVICE_RESUMED && !device) {
                auto candidate = ei_event_get_device(event);
                if (ei_device_has_capability(candidate, keyboard ? EI_DEVICE_CAP_KEYBOARD : (touch ? EI_DEVICE_CAP_TOUCH : EI_DEVICE_CAP_POINTER_ABSOLUTE))) {
                    device = ei_device_ref(candidate);
                    ei_device_start_emulating(device, 1);
                }
            }
            ei_event_unref(event);
        }
    };
    QElapsedTimer timer; timer.start();
    while (!device && timer.elapsed()<5000) {
        pollfd poller{ei_get_fd(context), POLLIN, 0}; poll(&poller, 1, 20); dispatch();
    }
    if (!device) fail("No matching emulated input device.");
    // Let clients bind newly advertised seat capabilities before sending input.
    for (int i=0;i<12;++i) { QThread::msleep(25); dispatch(); }
    if (!keyboard && !ei_device_get_region_at(device,x,y)) fail("Input point outside emulated output.");
    if ((action.contains("drag") || action=="two-touch") && !ei_device_get_region_at(device,x2,y2))
        fail("Input endpoint outside emulated output.");
    auto frame = [&] { ei_device_frame(device, now()); dispatch(); QThread::msleep(40); dispatch(); };
    auto hold = [&] {
        std::puts("pressed");std::fflush(stdout);
        for(int i=0;i<50;++i) { QThread::msleep(30);dispatch(); }
    };
    if (keyboard) {
        ei_device_keyboard_key(device,KEY_LEFTMETA,true);frame();
        ei_device_keyboard_key(device,KEY_TAB,true);frame();
        ei_device_keyboard_key(device,KEY_TAB,false);frame();
        ei_device_keyboard_key(device,KEY_LEFTMETA,false);frame();
    } else if (touch) {
        auto contact = ei_device_touch_new(device);
        ei_touch_down(contact, x, y); frame();
        if(action=="hold-touch")hold();
        ei_touch *second = nullptr;
        if (action == "two-touch") {
            second = ei_device_touch_new(device); ei_touch_down(second, x2, y2); frame();
        }
        if (action == "touch-drag") {
            for (int i=1;i<=12;++i) { ei_touch_motion(contact, x+(x2-x)*i/12., y+(y2-y)*i/12.); frame(); }
        }
        ei_touch_up(contact); frame(); ei_touch_unref(contact);
        if (second) { ei_touch_up(second); frame(); ei_touch_unref(second); }
    } else {
        ei_device_pointer_motion_absolute(device, x, y); frame();
        if (action == "click" || action == "drag" || action == "hold-click") {
            ei_device_button_button(device, BTN_LEFT, true); frame();
            if(action=="hold-click")hold();
            if (action == "drag") {
                for (int i=1;i<=12;++i) { ei_device_pointer_motion_absolute(device, x+(x2-x)*i/12., y+(y2-y)*i/12.); frame(); }
            }
            ei_device_button_button(device, BTN_LEFT, false); frame();
        }
    }
    QThread::msleep(150); dispatch();
    ei_device_stop_emulating(device); dispatch();
    ei_device_unref(device); ei_unref(context);
    return 0;
}
