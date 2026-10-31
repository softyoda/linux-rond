// SPDX-License-Identifier: GPL-2.0-or-later
#include "input.h"
#include "inputcoordinates.h"

#include <input_event.h>
#include <pointer_input.h>
#include <touch_input.h>
#include <wayland/seat.h>
#include <wayland/surface.h>
#include <wayland_server.h>
#include <window.h>
#include <workspace.h>

#include <QMatrix4x4>
#include <QLoggingCategory>
#include <QTimer>
#include <utility>

namespace LinuxRond {
using namespace KWin;
Q_LOGGING_CATEGORY(inputLog, "linux.rond.input", QtInfoMsg)

RoundInputFilter::RoundInputFilter(InputCallbacks callbacks)
    : InputEventFilter(InputFilterOrder::DragAndDrop)
    , m_callbacks(std::move(callbacks))
{
    input()->installInputEventFilter(this);
    connect(waylandServer(), &WaylandServer::lockStateChanged, this, [this] {
        if (waylandServer()->isScreenLocked()) {
            clearState(false);
            m_capturedKeys.clear();
            m_suppressedButtons.clear();
            m_suppressedTouches.clear();
        }
    });
}

RoundInputFilter::~RoundInputFilter()
{
    cancel();
}

bool RoundInputFilter::overview() const
{
    return m_callbacks.overviewActive && m_callbacks.overviewActive();
}

bool RoundInputFilter::allowed()
{
    if (waylandServer()->isScreenLocked()) return false;
    if (!m_callbacks.enabled || !m_callbacks.enabled()) {
        clearState(true);
        return false;
    }
    const bool currentOverview = overview();
    if (currentOverview != m_wasOverview) {
        clearState(true);
        m_wasOverview = currentOverview;
    }
    return true;
}

bool RoundInputFilter::hasActiveCapture() const
{
    return !m_buttons.isEmpty() || !m_touches.isEmpty();
}

void RoundInputFilter::notifyCaptureFinished()
{
    if (hasActiveCapture() || !m_callbacks.captureFinished) return;
    QTimer::singleShot(0, this, [this] {
        if (!hasActiveCapture() && m_callbacks.captureFinished) m_callbacks.captureFinished();
    });
}

void RoundInputFilter::clearState(bool deliverReleases)
{
    const bool hadCapture = hasActiveCapture();
    auto seat = waylandServer()->seat();
    if (deliverReleases && !waylandServer()->isScreenLocked()) {
        if (m_pointerSurface && seat->focusedPointerSurface() == m_pointerSurface) {
            for (quint32 button : std::as_const(m_deliveredButtons)) {
                seat->notifyPointerButton(button, PointerButtonState::Released);
            }
            if (!m_deliveredButtons.isEmpty()) seat->notifyPointerFrame();
        }
        bool deliveredTouch = false;
        for (const auto &contact : std::as_const(m_touches)) deliveredTouch |= contact.delivered;
        if (deliveredTouch) seat->notifyTouchCancel();
    }
    m_suppressedButtons.unite(m_buttons);
    for (auto it = m_touches.cbegin(); it != m_touches.cend(); ++it) m_suppressedTouches.insert(it.key());
    m_buttons.clear();
    m_deliveredButtons.clear();
    m_touches.clear();
    m_pointerWindow = nullptr;
    m_pointerSurface = nullptr;
    m_overviewTouch.reset();
    m_overviewPointer = false;
    m_pointerFramePending = false;
    m_touchFramePending = false;
    if (hadCapture) notifyCaptureFinished();
}

void RoundInputFilter::cancel()
{
    clearState(true);
}

std::optional<InputTarget> RoundInputFilter::pointerTarget(QPointF position)
{
    if (!m_buttons.isEmpty()) {
        if (m_pointerWindow && m_callbacks.mapToSource) {
            if (auto mapped = m_callbacks.mapToSource(m_pointerWindow, position)) {
                return InputTarget{m_pointerWindow, *mapped};
            }
        }
        return InputTarget{m_pointerWindow, m_lastPointerSource};
    }
    return m_callbacks.hitTest ? m_callbacks.hitTest(position) : std::nullopt;
}

bool RoundInputFilter::focusPointer(const InputTarget &target)
{
    auto window = target.window.data();
    if (!window || !window->surface()) return false;
    auto seat = waylandServer()->seat();
    auto surface = window->surface();
    const auto transform = window->inputTransformation();
    // Keep the physical cursor unchanged; only client coordinates are mapped.
    input()->pointer()->setFocus(window);
    if (seat->focusedPointerSurface() != surface) {
        seat->notifyPointerEnter(surface, target.sourceGlobal, transform);
    } else {
        seat->setFocusedPointerSurfaceTransformation(transform);
    }
    m_pointerWindow = window;
    m_pointerSurface = surface;
    m_lastPointerSource = target.sourceGlobal;
    qCDebug(inputLog) << "pointer source/local" << target.sourceGlobal << transform.map(target.sourceGlobal);
    return true;
}

bool RoundInputFilter::dismissPopup(Window *target)
{
    const auto windows = workspace()->stackingOrder();
    bool dismissed = false;
    for (auto it = windows.crbegin(); it != windows.crend(); ++it) {
        auto popup = *it;
        if (!popup->hasPopupGrab()) continue;
        if (target && Window::belongToSameApplication(target, popup)) return false;
        popup->popupDone();
        dismissed = true;
    }
    return dismissed;
}

bool RoundInputFilter::dragMotion(QPointF position, std::chrono::microseconds time)
{
    auto seat = waylandServer()->seat();
    auto target = m_callbacks.hitTest ? m_callbacks.hitTest(position) : std::nullopt;
    if (!target) return false;
    seat->setTimestamp(time);
    if (!target->window || !target->window->surface()) {
        seat->setDragTarget(nullptr, nullptr, position, QMatrix4x4());
        return true;
    }
    auto surface = target->window->surface();
    auto handler = seat->dropHandlerForSurface(surface);
    if (!handler) {
        // Native XWayland DND needs its bridge; do not send unwarped coordinates.
        seat->setDragTarget(nullptr, nullptr, position, QMatrix4x4());
        return true;
    }
    seat->setDragTarget(handler, surface, target->sourceGlobal, target->window->inputTransformation());
    seat->notifyDragMotion(target->sourceGlobal);
    return true;
}

bool RoundInputFilter::pointerMotion(PointerMotionEvent *event)
{
    if (!allowed()) return false;
    if (overview()) {
        if (!m_overviewPointer && (!m_callbacks.hitTest || !m_callbacks.hitTest(event->position))) return false;
        if (m_overviewPointer && m_callbacks.overviewMove) m_callbacks.overviewMove(event->position);
        return true;
    }
    auto seat = waylandServer()->seat();
    if (seat->isDragPointer()) return dragMotion(event->position, event->timestamp);
    auto target = pointerTarget(event->position);
    if (!target) return false;
    seat->setTimestamp(event->timestamp);
    if (!focusPointer(*target)) {
        if (m_buttons.isEmpty()) seat->notifyPointerLeave();
        return true;
    }
    if (event->warp) seat->notifyPointerWarp(target->sourceGlobal);
    else {
        seat->notifyPointerMotion(target->sourceGlobal);
        if (!event->delta.isNull()) {
            auto mapDelta = [&](QPointF delta) {
                if (m_callbacks.mapToSource) {
                    if (auto previous = m_callbacks.mapToSource(target->window, event->position - delta)) {
                        return target->sourceGlobal - *previous;
                    }
                }
                return delta;
            };
            seat->relativePointerMotion(mapDelta(event->delta), mapDelta(event->deltaUnaccelerated), event->timestamp);
        }
    }
    m_pointerFramePending = true;
    return true;
}

bool RoundInputFilter::pointerButton(PointerButtonEvent *event)
{
    const bool pressed = event->state == PointerButtonState::Pressed;
    if (pressed) m_suppressedButtons.remove(event->nativeButton);
    else if (m_suppressedButtons.remove(event->nativeButton)) return true;
    if (!allowed()) return !pressed && m_suppressedButtons.remove(event->nativeButton);
    if (!pressed && m_suppressedButtons.remove(event->nativeButton)) return true;
    if (overview()) {
        if (pressed && (!m_callbacks.hitTest || !m_callbacks.hitTest(event->position))) return false;
        if (!pressed && !m_buttons.contains(event->nativeButton)) return false;
        if (pressed) {
            m_buttons.insert(event->nativeButton);
            if (event->button == Qt::LeftButton && !m_overviewPointer) {
                m_overviewPointer = true;
                if (m_callbacks.overviewPress) m_callbacks.overviewPress(event->position);
            }
        } else if (m_buttons.remove(event->nativeButton) && event->button == Qt::LeftButton && m_overviewPointer) {
            m_overviewPointer = false;
            if (m_callbacks.overviewRelease) m_callbacks.overviewRelease(event->position);
        }
        if (!pressed) notifyCaptureFinished();
        return true;
    }
    if (!pressed && !m_buttons.contains(event->nativeButton)) return false;
    auto target = pointerTarget(event->position);
    if (!target) return false;
    if (pressed) {
        if (m_buttons.isEmpty()) {
            m_pointerWindow = target->window;
            m_pointerSurface = target->window ? target->window->surface() : nullptr;
        }
        m_buttons.insert(event->nativeButton);
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(event->timestamp);
    if (pressed && dismissPopup(target->window)) {
        m_pointerWindow = nullptr;
        m_pointerSurface = nullptr;
        return true;
    }
    if (focusPointer(*target)) {
        if (pressed) {
            if (!target->window->isPopupWindow()) workspace()->activateWindow(target->window);
            workspace()->setActiveOutput(event->position);
            m_deliveredButtons.insert(event->nativeButton);
        }
        if (pressed || m_deliveredButtons.contains(event->nativeButton)) {
            seat->notifyPointerMotion(target->sourceGlobal);
            seat->notifyPointerButton(event->nativeButton, event->state);
            m_pointerFramePending = true;
        }
    } else if (!pressed && m_deliveredButtons.contains(event->nativeButton)) {
        // A destroyed grab target must not leave the seat button pressed.
        input()->pointer()->setFocus(nullptr);
        seat->notifyPointerButton(event->nativeButton, PointerButtonState::Released);
        input()->pointer()->update();
        m_pointerFramePending = true;
    }
    if (!pressed) {
        m_buttons.remove(event->nativeButton);
        m_deliveredButtons.remove(event->nativeButton);
        notifyCaptureFinished();
    }
    return true;
}

bool RoundInputFilter::pointerAxis(PointerAxisEvent *event)
{
    if (!allowed()) return false;
    if (overview()) return m_callbacks.hitTest && m_callbacks.hitTest(event->position).has_value();
    auto target = pointerTarget(event->position);
    if (!target) return false;
    if (focusPointer(*target)) {
        auto seat = waylandServer()->seat();
        seat->setTimestamp(event->timestamp);
        seat->notifyPointerMotion(target->sourceGlobal);
        seat->notifyPointerAxis(event->orientation, event->delta, event->deltaV120, event->source, event->inverted);
        m_pointerFramePending = true;
    }
    return true;
}

bool RoundInputFilter::pointerFrame()
{
    if (!allowed()) return false;
    if (!m_pointerFramePending) return false;
    m_pointerFramePending = false;
    waylandServer()->seat()->notifyPointerFrame();
    return true;
}

bool RoundInputFilter::touchDown(TouchDownEvent *event)
{
    m_suppressedTouches.remove(event->id);
    if (!allowed()) return false;
    if (m_touches.contains(event->id)) return true;
    if (overview()) {
        if (!m_callbacks.hitTest || !m_callbacks.hitTest(event->pos)) return false;
        m_touches.insert(event->id, TouchContact{{}, event->pos, false, true});
        if (!m_overviewTouch) {
            m_overviewTouch = event->id;
            if (m_callbacks.overviewPress) m_callbacks.overviewPress(event->pos);
        }
        return true;
    }
    auto target = m_callbacks.hitTest ? m_callbacks.hitTest(event->pos) : std::nullopt;
    if (!target) return false;
    TouchContact contact{target->window, target->sourceGlobal};
    if (dismissPopup(target->window)) contact.window = nullptr;
    if (contact.window && contact.window->surface()) {
        auto seat = waylandServer()->seat();
        seat->setTimestamp(event->time);
        if (!seat->isTouchSequence()) {
            if (!contact.window->isPopupWindow()) workspace()->activateWindow(contact.window);
            workspace()->setActiveOutput(event->pos);
            input()->touch()->setFocus(contact.window);
        }
        contact.initialBufferOrigin = contact.window->bufferGeometry().topLeft();
        contact.delivered = seat->notifyTouchDown(contact.window->surface(),
            contact.initialBufferOrigin, event->id, contact.lastSource) != nullptr;
        m_touchFramePending |= contact.delivered;
        qCDebug(inputLog) << "touch down" << event->id << event->pos << contact.lastSource << contact.delivered;
    }
    m_touches.insert(event->id, contact);
    return true;
}

bool RoundInputFilter::touchMotion(TouchMotionEvent *event)
{
    if (m_suppressedTouches.contains(event->id)) return true;
    if (!allowed()) return m_suppressedTouches.contains(event->id);
    if (m_suppressedTouches.contains(event->id)) return true;
    auto it = m_touches.find(event->id);
    if (it == m_touches.end()) return false;
    if (it->overview) {
        it->lastSource = event->pos;
        if (m_overviewTouch == event->id && m_callbacks.overviewMove) m_callbacks.overviewMove(event->pos);
        return true;
    }
    auto seat = waylandServer()->seat();
    if (seat->isDragTouch()) {
        const auto serial = seat->dragSerial();
        const auto point = serial ? seat->touchPointByImplicitGrabSerial(*serial) : nullptr;
        if (!point || point->id != event->id) return true;
        return dragMotion(event->pos, event->time);
    }
    if (it->window && it->delivered) {
        if (m_callbacks.mapToSource) {
            if (auto point = m_callbacks.mapToSource(it->window, event->pos)) it->lastSource = *point;
        }
        seat->setTimestamp(event->time);
        seat->notifyTouchMotion(event->id, touchSeatPosition(it->lastSource,
            it->window->bufferGeometry().topLeft(), it->initialBufferOrigin));
        m_touchFramePending = true;
    }
    return true;
}

bool RoundInputFilter::touchUp(TouchUpEvent *event)
{
    if (m_suppressedTouches.remove(event->id)) return true;
    if (!allowed()) return m_suppressedTouches.remove(event->id);
    if (m_suppressedTouches.remove(event->id)) return true;
    auto it = m_touches.find(event->id);
    if (it == m_touches.end()) return false;
    const auto contact = *it;
    m_touches.erase(it);
    if (contact.overview) {
        if (m_overviewTouch == event->id) {
            m_overviewTouch.reset();
            if (m_callbacks.overviewRelease) m_callbacks.overviewRelease(contact.lastSource);
        }
    } else if (contact.delivered) {
        auto seat = waylandServer()->seat();
        seat->setTimestamp(event->time);
        if (seat->isDragTouch()) {
            const auto serial = seat->dragSerial();
            const auto point = serial ? seat->touchPointByImplicitGrabSerial(*serial) : nullptr;
            if (point && point->id == event->id) seat->endDrag();
        }
        seat->notifyTouchUp(event->id);
        m_touchFramePending = true;
    }
    notifyCaptureFinished();
    return true;
}

bool RoundInputFilter::touchFrame()
{
    if (!allowed()) return false;
    if (!m_touchFramePending) return false;
    m_touchFramePending = false;
    waylandServer()->seat()->notifyTouchFrame();
    // Native contacts on other outputs may share this frame.
    return false;
}

bool RoundInputFilter::touchCancel()
{
    m_suppressedTouches.clear();
    if (m_touches.isEmpty()) return false;
    const bool wasOverview = m_overviewTouch.has_value();
    clearState(true);
    m_suppressedTouches.clear();
    if (wasOverview && m_callbacks.overviewCancel) m_callbacks.overviewCancel();
    return false;
}

bool RoundInputFilter::keyboardKey(KeyboardKeyEvent *event)
{
    if (m_capturedKeys.contains(event->key)) {
        if (event->state == KeyboardKeyState::Released) m_capturedKeys.remove(event->key);
        return true;
    }
    if (!allowed()) return false;
    if (event->key == Qt::Key_Tab && event->state == KeyboardKeyState::Pressed
        && event->modifiers.testFlag(Qt::MetaModifier) && m_callbacks.toggleOverview) {
        m_capturedKeys.insert(event->key);
        m_callbacks.toggleOverview();
        return true;
    }
    if (overview() && event->key == Qt::Key_Escape && event->state == KeyboardKeyState::Pressed) {
        m_capturedKeys.insert(event->key);
        if (m_callbacks.overviewCancel) m_callbacks.overviewCancel();
        return true;
    }
    return false;
}

}
