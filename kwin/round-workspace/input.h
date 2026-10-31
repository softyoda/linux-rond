// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <input.h>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <functional>
#include <optional>

namespace KWin {
class SurfaceInterface;
class Window;
}

namespace LinuxRond {

struct InputTarget {
    QPointer<KWin::Window> window;
    QPointF sourceGlobal;
};

struct InputCallbacks {
    std::function<bool()> enabled;
    // nullopt: native input; null window: swallow empty projected canvas.
    std::function<std::optional<InputTarget>(QPointF)> hitTest;
    std::function<std::optional<QPointF>(KWin::Window *, QPointF)> mapToSource;
    std::function<bool()> overviewActive;
    std::function<void()> toggleOverview;
    std::function<void(QPointF)> overviewPress;
    std::function<void(QPointF)> overviewMove;
    std::function<void(QPointF)> overviewRelease;
    std::function<void()> overviewCancel;
    std::function<void()> captureFinished;
};

class RoundInputFilter final : public QObject, public KWin::InputEventFilter {
public:
    explicit RoundInputFilter(InputCallbacks callbacks);
    ~RoundInputFilter() override;
    void cancel();
    bool hasActiveCapture() const;

    bool pointerMotion(KWin::PointerMotionEvent *event) override;
    bool pointerButton(KWin::PointerButtonEvent *event) override;
    bool pointerAxis(KWin::PointerAxisEvent *event) override;
    bool pointerFrame() override;
    bool touchDown(KWin::TouchDownEvent *event) override;
    bool touchMotion(KWin::TouchMotionEvent *event) override;
    bool touchUp(KWin::TouchUpEvent *event) override;
    bool touchFrame() override;
    bool touchCancel() override;
    bool keyboardKey(KWin::KeyboardKeyEvent *event) override;

private:
    struct TouchContact {
        QPointer<KWin::Window> window;
        QPointF lastSource;
        bool delivered = false;
        bool overview = false;
        QPointF initialBufferOrigin{};
    };
    bool allowed();
    bool overview() const;
    std::optional<InputTarget> pointerTarget(QPointF position);
    bool focusPointer(const InputTarget &target);
    bool dismissPopup(KWin::Window *target);
    bool dragMotion(QPointF position, std::chrono::microseconds time);
    void clearState(bool deliverReleases);
    void notifyCaptureFinished();

    InputCallbacks m_callbacks;
    QPointer<KWin::Window> m_pointerWindow;
    QPointer<KWin::SurfaceInterface> m_pointerSurface;
    QPointF m_lastPointerSource;
    QSet<quint32> m_buttons;
    QSet<quint32> m_deliveredButtons;
    QSet<quint32> m_suppressedButtons;
    QSet<qint32> m_suppressedTouches;
    QHash<qint32, TouchContact> m_touches;
    std::optional<qint32> m_overviewTouch;
    bool m_overviewPointer = false;
    bool m_pointerFramePending = false;
    bool m_touchFramePending = false;
    bool m_wasOverview = false;
    QSet<Qt::Key> m_capturedKeys;
};

}
