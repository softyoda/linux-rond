// SPDX-License-Identifier: GPL-2.0-or-later
#include "workspace.h"
#include <QDBusConnection>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <core/output.h>
#include <effect/effecthandler.h>
#include <utility>
#include <window.h>
#include <workspace.h>

using namespace KWin;
namespace LinuxRond {
namespace {
QString settingsFile() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/linux-rondrc";
}
QJsonObject jsonRect(QRectF r) {
    return {{"x", r.x()}, {"y", r.y()}, {"width", r.width()}, {"height", r.height()}};
}
QRectF inscribed(QRectF r) {
    const double side = std::min(r.width(), r.height()) / std::sqrt(2.);
    return {r.center().x() - side / 2, r.center().y() - side / 2, side, side};
}
} // namespace
WorkspaceEffect::WorkspaceEffect()
    : m_settings(settingsFile(), QSettings::IniFormat), m_preferences(settingsFile()) {
    QFile file(":/linux-rond/approved-mesh.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Linux Rond: projection resource missing");
        return;
    }
    const auto json = file.readAll();
    try {
        for (int i = 0; i < int(m_meshes.size()); ++i)
            m_meshes[i] = WarpMesh::fromJson(json, Shape(i));
    } catch (const std::exception &e) {
        qWarning("Linux Rond: %s", e.what());
        return;
    }
    setVertexSnappingMode(RenderGeometry::VertexSnappingMode::None);
    InputCallbacks callbacks;
    callbacks.enabled = [this] { return isActive(); };
    callbacks.hitTest = [this](QPointF p) { return hitTest(p); };
    callbacks.mapToSource = [this](Window *w, QPointF p) { return mapToSource(w, p); };
    callbacks.toggleOverview = [this] { ToggleOverview(); };
    callbacks.captureFinished = [this] { QTimer::singleShot(0, this, &WorkspaceEffect::drainDeferred); };
    callbacks.overviewActive = [this] { return m_overview; };
    callbacks.overviewPress = [this](QPointF p) { m_gesture.begin(p, m_placements, content()); };
    callbacks.overviewMove = [this](QPointF p) {
        m_gesture.update(p);
        ++m_generation;
        effects->addRepaintFull();
    };
    callbacks.overviewRelease = [this](QPointF p) { overviewRelease(p); };
    callbacks.overviewCancel = [this] {
        if (m_selecting)
            ExitSplit();
        else {
            m_overview = false;
            refreshLayout();
        }
    };
    m_input = std::make_unique<RoundInputFilter>(callbacks);
    connect(effects, &EffectsHandler::screenLockingChanged, this, [this](bool locked) {
        if (locked) {
            m_deferred.clear();
            m_gesture.cancel();
        }
    });
    connect(workspace(), &Workspace::windowAdded, this, &WorkspaceEffect::observe);
    connect(workspace(), &Workspace::windowRemoved, this, [this](Window *w) {
        if (auto ew = w->effectWindow()) {
            m_visible.erase(ew);
            m_quads.remove(ew);
            m_redirected.remove(ew);
        }
        m_windows.remove(w);
        m_observed.remove(w);
        m_pendingFit.remove(w);
        m_fitAttempts.remove(w);
        if (m_splitLeft == w || m_splitRight == w) {
            m_splitLeft.clear();
            m_splitRight.clear();
            m_selecting = false;
            refitAll();
        }
        if (m_lastActive == w)
            m_lastActive.clear();
        refreshLayout();
    });
    connect(workspace(), &Workspace::windowActivated, this, [this](Window *w) {
        if (auto root = rootFor(w))
            m_lastActive = root;
        changed();
    });
    connect(workspace(), &Workspace::outputAdded, this, [this] { refreshOutput(); });
    connect(workspace(), &Workspace::outputRemoved, this, [this] { refreshOutput(); });
    connect(effects, &EffectsHandler::windowDeleted, this, [this](EffectWindow *w) {
        m_visible.erase(w);
        m_redirected.remove(w);
        m_quads.remove(w);
    });
    m_ready = QDBusConnection::sessionBus().registerObject(
        "/LinuxRond", this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportAllSignals);
    if (!m_ready) {
        qWarning("Linux Rond: DBus object already registered");
        return;
    }
    for (auto w : workspace()->windows())
        observe(w);
    refreshOutput();
    const bool requested = m_settings.value("Workspace/Enabled", false).toBool();
    if (requested)
        SetEnabled(true);
}
WorkspaceEffect::~WorkspaceEffect() {
    m_enabled = false;
    if (m_input)
        m_input->cancel();
    m_input.reset();
    restore();
    if (m_ready)
        QDBusConnection::sessionBus().unregisterObject("/LinuxRond");
}
bool WorkspaceEffect::deferPresentation(std::function<void()> action) {
    if (!m_input || !m_input->hasActiveCapture())
        return false;
    m_deferred.push_back(std::move(action));
    Q_EMIT StateChanged(StateJson());
    return true;
}
void WorkspaceEffect::drainDeferred() {
    if (effects->isScreenLocked()) {
        m_deferred.clear();
        return;
    }
    if (m_input && m_input->hasActiveCapture())
        return;
    const auto actions = std::exchange(m_deferred, {});
    for (const auto &action : actions)
        action();
}
bool WorkspaceEffect::isActive() const {
    return m_enabled && m_output && m_ready && !effects->isScreenLocked();
}
const WarpMesh &WorkspaceEffect::mesh(Shape shape) const { return m_meshes[int(shape)]; }
QString WorkspaceEffect::id(Window *w) const {
    return w ? w->internalId().toString(QUuid::WithoutBraces) : QString();
}
QString WorkspaceEffect::appId(Window *w) const {
    return w->desktopFileName().isEmpty() ? w->resourceClass() : w->desktopFileName();
}
Window *WorkspaceEffect::find(const QString &key) const {
    for (auto it = m_windows.cbegin(); it != m_windows.cend(); ++it)
        if (id(it.key()) == key)
            return it.key();
    return nullptr;
}
Window *WorkspaceEffect::rootFor(Window *w) const {
    QSet<Window *> seen;
    while (w && !seen.contains(w)) {
        if (m_windows.contains(w))
            return w;
        seen.insert(w);
        w = w->transientFor();
    }
    return nullptr;
}
Window *WorkspaceEffect::active() const {
    if (auto w = rootFor(workspace()->activeWindow()))
        return w;
    if (m_lastActive && m_windows.contains(m_lastActive))
        return m_lastActive;
    const auto stack = workspace()->stackingOrder();
    for (auto it = stack.crbegin(); it != stack.crend(); ++it)
        if (m_windows.contains(*it) && !(*it)->isMinimized())
            return *it;
    return nullptr;
}
bool WorkspaceEffect::eligible(Window *w) const {
    return m_output && w->output() == m_output && w->isNormalWindow() && !w->isTransient() &&
           !w->isInternal() && !w->isLockScreen() && !w->isInputMethod();
}
QRectF WorkspaceEffect::canvas() const { return m_output ? QRectF(m_output->geometryF()) : QRectF(); }
QRectF WorkspaceEffect::sourceCanvas(Window *window) const {
    auto rect = canvas();
    if (window == m_splitLeft || window == m_splitRight) {
        rect.setWidth(rect.width() / 2);
        if (window == m_splitRight)
            rect.translate(rect.width(), 0);
    }
    return rect;
}
void WorkspaceEffect::refitAll() {
    for (auto window : m_windows.keys())
        fit(window);
}
QRectF WorkspaceEffect::content() const {
    auto rect = canvas();
    const double d = std::min(rect.width(), rect.height());
    const double margin = std::clamp(m_settings.value("Workspace/RimMargin", 64.).toDouble(), 0., d * .25);
    const double side = d - 2 * margin;
    return {rect.center().x() - side / 2, rect.center().y() - side / 2, side, side};
}
void WorkspaceEffect::refreshOutput() {
    LogicalOutput *chosen = nullptr;
    int matches = 0;
    const auto name = m_settings.value("Workspace/OutputName").toString();
    for (auto out : workspace()->outputs()) {
        const auto r = out->geometryF();
        if ((!name.isEmpty() && out->name() == name) ||
            (name.isEmpty() && qFuzzyCompare(r.width(), r.height()))) {
            chosen = out;
            ++matches;
        }
    }
    if (matches != 1)
        chosen = nullptr;
    if (m_output != chosen) {
        if (m_input)
            m_input->cancel();
        restore();
        if (m_output)
            disconnect(m_output, nullptr, this, nullptr);
        m_output = chosen;
        if (m_output)
            connect(m_output, &LogicalOutput::geometryChanged, this, [this] {
                if (m_input)
                    m_input->cancel();
                for (auto w : m_windows.keys())
                    fit(w);
                refreshLayout();
            });
        if (m_enabled)
            for (auto w : workspace()->windows())
                manage(w);
    }
    refreshLayout();
}
void WorkspaceEffect::observe(Window *w) {
    if (m_observed.contains(w))
        return;
    m_observed.insert(w);
    connect(w, &Window::frameGeometryChanged, this, [this, w] {
        if (m_enabled && !m_busy && !m_pendingFit.contains(w) && m_windows.contains(w))
            fit(w);
        m_quads.remove(w->effectWindow());
    });
    connect(w, &Window::outputChanged, this, [this, w] {
        if (!m_enabled || m_busy)
            return;
        if (m_windows.contains(w))
            fit(w);
        else
            manage(w);
        refreshLayout();
    });
    connect(w, &Window::interactiveMoveResizeStarted, this, [this, w] {
        if (m_enabled && m_windows.contains(w)) {
            w->cancelInteractiveMoveResize();
            fit(w);
        }
    });
    connect(w, &Window::maximizedChanged, this, [this, w] {
        if (m_enabled && !m_busy && m_windows.contains(w) && w->maximizeMode() != MaximizeRestore)
            ToggleMode(id(w));
    });
    connect(w, &Window::fullScreenChanged, this, [this, w] {
        if (m_enabled && !m_busy && m_windows.contains(w))
            fit(w);
    });
    connect(w, &Window::minimizedChanged, this, [this] { refreshLayout(); });
    connect(w, &Window::captionChanged, this, [this] { changed(); });
    const auto restoreMode = [this, w] {
        if (m_windows.contains(w)) {
            m_windows[w].circular = m_preferences.circular(appId(w));
            refreshLayout();
        }
    };
    connect(w, &Window::desktopFileNameChanged, this, restoreMode);
    connect(w, &Window::windowClassChanged, this, restoreMode);
    if (m_enabled) {
        manage(w);
        refreshLayout();
    }
}
void WorkspaceEffect::manage(Window *w) {
    if (!eligible(w) || m_windows.contains(w))
        return;
    SavedWindow saved{w,
                      QRectF(w->frameGeometry()),
                      w->noBorder(),
                      w->isFullScreen(),
                      m_preferences.circular(appId(w)),
                      w->maximizeMode()};
    m_windows.insert(w, saved);
    fit(w);
}
void WorkspaceEffect::fit(Window *w, bool retry) {
    if (m_busy || !m_output || !m_windows.contains(w))
        return;
    if (!retry)
        m_fitAttempts[w] = 0;
    m_busy = true;
    if (w->isInteractiveMove() || w->isInteractiveResize())
        w->cancelInteractiveMoveResize();
    if (w->isFullScreen())
        w->setFullScreen(false);
    if (w->maximizeMode() != MaximizeRestore)
        w->setMaximize(false, false);
    w->setNoBorder(true);
    if (QRectF(w->frameGeometry()) != sourceCanvas(w)) {
        m_pendingFit.insert(w);
        ++m_fitAttempts[w];
        w->moveResize(RectF(sourceCanvas(w)));
        QTimer::singleShot(250, this, [this, window = QPointer<Window>(w)] {
            if (window) {
                m_pendingFit.remove(window);
                if (m_enabled && m_windows.contains(window) && m_fitAttempts.value(window) < 3 &&
                    QRectF(window->frameGeometry()) != sourceCanvas(window))
                    fit(window, true);
                changed();
            }
        });
    }
    m_busy = false;
}
void WorkspaceEffect::restore() {
    m_busy = true;
    m_visible.clear();
    for (auto ew : std::as_const(m_redirected))
        unredirect(ew);
    m_redirected.clear();
    m_quads.clear();
    for (const auto &saved : std::as_const(m_windows))
        if (saved.window) {
            auto w = saved.window.data();
            w->setNoBorder(saved.noBorder);
            w->moveResize(RectF(saved.geometry));
            if (saved.maximize != MaximizeRestore)
                w->maximize(saved.maximize);
            if (saved.fullscreen)
                w->setFullScreen(true);
        }
    m_windows.clear();
    m_pendingFit.clear();
    m_fitAttempts.clear();
    m_lastActive.clear();
    m_splitLeft.clear();
    m_splitRight.clear();
    m_placements.clear();
    m_overview = false;
    m_selecting = false;
    m_gesture.cancel();
    m_busy = false;
    effects->addRepaintFull();
}
void WorkspaceEffect::SetEnabled(bool value) {
    if (deferPresentation([this, value] { SetEnabled(value); }))
        return;
    if (!m_ready || m_enabled == value)
        return;
    if (m_input)
        m_input->cancel();
    m_enabled = value;
    m_settings.setValue("Workspace/Enabled", value);
    m_settings.sync();
    if (value) {
        refreshOutput();
        for (auto w : workspace()->windows())
            manage(w);
    } else
        restore();
    refreshLayout();
}
void WorkspaceEffect::changed() {
    ++m_generation;
    effects->addRepaintFull();
    Q_EMIT StateChanged(StateJson());
}
void WorkspaceEffect::refreshLayout() {
    m_placements.clear();
    if (m_overview && m_output) {
        QVector<OverviewWindow> windows;
        for (auto w : workspace()->stackingOrder())
            if (m_windows.contains(w) && w != m_splitLeft && w != m_splitRight)
                windows.push_back({id(w), m_windows[w].circular, w->isMinimized(), w == active()});
        const auto region = !m_selecting ? OverviewRegion::Full
                                         : (m_splitLeft ? OverviewRegion::Right : OverviewRegion::Left);
        m_placements = packOverview(windows, content(), region);
    }
    for (auto w : workspace()->windows()) {
        auto ew = w->effectWindow();
        if (!ew)
            continue;
        const bool visible = m_overview && rootFor(w);
        if (visible && !m_visible.contains(ew))
            m_visible.emplace(ew, std::make_unique<EffectWindowVisibleRef>(
                                      ew, EffectWindow::PAINT_DISABLED_BY_MINIMIZE |
                                              EffectWindow::PAINT_DISABLED_BY_DESKTOP |
                                              EffectWindow::PAINT_DISABLED_BY_ACTIVITY));
        if (!visible)
            m_visible.erase(ew);
    }
    changed();
}
std::optional<WorkspaceEffect::Presentation> WorkspaceEffect::presentation(Window *w) const {
    auto root = rootFor(w);
    if (!root || !m_output)
        return std::nullopt;
    Presentation result{sourceCanvas(root), content(),
                        m_windows[root].circular ? Shape::Circle : Shape::Square};
    if (root == m_splitLeft) {
        result.shape = Shape::LeftHalf;
        return result;
    }
    if (root == m_splitRight) {
        result.shape = Shape::RightHalf;
        return result;
    }
    if (m_overview) {
        for (const auto &p : m_placements)
            if (p.id == id(root)) {
                result.destination = p.bounds;
                if (!p.circular)
                    result.shape = Shape::RoundedSquare;
                if (m_gesture.dragging() && m_gesture.windowId() == p.id)
                    result.destination.moveCenter(m_gesture.position());
                return result;
            }
        return std::nullopt;
    }
    if (m_splitLeft || m_splitRight || root != active() || root->isMinimized())
        return std::nullopt;
    if (result.shape == Shape::Square)
        result.destination = inscribed(result.destination);
    return result;
}
void WorkspaceEffect::prePaintScreen(ScreenPrePaintData &data) {
    if (isActive())
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    effects->prePaintScreen(data);
}
void WorkspaceEffect::prePaintWindow(RenderView *view, EffectWindow *ew, WindowPrePaintData &data) {
    if (isActive() && rootFor(ew->window())) {
        if (!m_redirected.contains(ew)) {
            redirect(ew);
            m_redirected.insert(ew);
        }
        data.setTransformed();
        data.setTranslucent();
    }
    effects->prePaintWindow(view, ew, data);
}
bool WorkspaceEffect::paintWindow(const RenderTarget &target, const RenderViewport &viewport,
                                  EffectWindow *ew, int mask, const Region &region, WindowPaintData &data) {
    if (isActive() && rootFor(ew->window()) && !presentation(ew->window()))
        return true;
    return effects->paintWindow(target, viewport, ew, mask, region, data);
}
void WorkspaceEffect::apply(EffectWindow *ew, int, WindowPaintData &data, WindowQuadList &quads) {
    if (!isActive())
        return;
    auto info = presentation(ew->window());
    if (!info) {
        quads.clear();
        return;
    }
    auto &cache = m_quads[ew];
    const QRectF frame = ew->frameGeometry(), expanded = ew->expandedGeometry();
    if (cache.generation != m_generation || cache.frame != frame || cache.expanded != expanded) {
        cache = {m_generation, frame, expanded, {}};
        const auto clipped = frame.intersected(info->source);
        if (clipped.isEmpty()) {
            quads.clear();
            return;
        }
        const QRectF normalized(toNormalized(clipped.topLeft(), info->source),
                                toNormalized(clipped.bottomRight(), info->source));
        for (const auto &triangle : mesh(info->shape).clipped(normalized)) {
            WindowQuad quad;
            for (int k = 0; k < 4; ++k) {
                const auto &vertex = triangle[std::min(k, 2)];
                const QPointF source = fromNormalized(vertex.source, info->source);
                const QPointF destination = fromNormalized(vertex.destination, info->destination);
                const QPointF texture((source.x() - expanded.x()) / expanded.width(),
                                      (source.y() - expanded.y()) / expanded.height());
                quad[k] = WindowVertex(destination - frame.topLeft(), texture);
            }
            cache.quads.append(quad);
        }
    }
    quads = cache.quads;
    data.setXScale(1);
    data.setYScale(1);
    data.setXTranslation(0);
    data.setYTranslation(0);
}
std::optional<QPointF> WorkspaceEffect::mapToSource(Window *w, QPointF point, bool clamp) const {
    auto info = presentation(w);
    if (!info)
        return std::nullopt;
    auto normalized = mesh(info->shape).inverse(toNormalized(point, info->destination), clamp);
    if (!normalized)
        return std::nullopt;
    return fromNormalized(*normalized, info->source);
}
std::optional<InputTarget> WorkspaceEffect::hitTest(QPointF point) const {
    if (!isActive() || !canvas().contains(point))
        return std::nullopt;
    const auto stack = workspace()->stackingOrder();
    for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
        Window *w = *it;
        if (!rootFor(w)) {
            const auto p = toNormalized(point, content());
            const bool blankArcPanel = w->isDock() && p.x() * p.x() + p.y() * p.y() <= 1;
            if (!blankArcPanel && w->isShown() && !w->isDesktop() && w->hitTest(point))
                return std::nullopt;
            continue;
        }
        if (m_overview)
            continue;
        if (auto source = mapToSource(w, point, false); source && w->hitTest(*source))
            return InputTarget{w, *source};
    }
    return InputTarget{};
}
void WorkspaceEffect::ToggleMode(const QString &key) {
    if (deferPresentation([this, key] { ToggleMode(key); }))
        return;
    auto w = find(key);
    if (!w)
        return;
    if (m_splitLeft || m_splitRight) {
        ExitSplitFor(key);
        return;
    }
    if (m_input)
        m_input->cancel();
    const bool circular = !m_windows[w].circular;
    const auto app = appId(w);
    m_preferences.setCircular(app, circular);
    for (auto it = m_windows.begin(); it != m_windows.end(); ++it)
        if (it.key() == w || (!app.isEmpty() && appId(it.key()) == app))
            it->circular = circular;
    fit(w);
    refreshLayout();
}
void WorkspaceEffect::ToggleActiveMode() { ToggleMode(id(active())); }
void WorkspaceEffect::Minimize(const QString &key) {
    if (deferPresentation([this, key] { Minimize(key); }))
        return;
    if (auto w = find(key)) {
        if (m_input)
            m_input->cancel();
        if (w == m_splitLeft || w == m_splitRight)
            ExitSplit();
        w->setMinimized(true);
        refreshLayout();
    }
}
void WorkspaceEffect::Close(const QString &key) {
    if (deferPresentation([this, key] { Close(key); }))
        return;
    if (auto w = find(key))
        w->closeWindow();
}
void WorkspaceEffect::MinimizeActive() { Minimize(id(active())); }
void WorkspaceEffect::CloseActive() { Close(id(active())); }
void WorkspaceEffect::Activate(const QString &key) {
    if (deferPresentation([this, key] { Activate(key); }))
        return;
    auto w = find(key);
    if (!w)
        return;
    if (m_selecting) {
        ChooseSplit(key);
        return;
    }
    if (m_input)
        m_input->cancel();
    m_overview = false;
    m_gesture.cancel();
    w->setMinimized(false);
    workspace()->activateWindow(w, true);
    m_lastActive = w;
    refreshLayout();
}
void WorkspaceEffect::ToggleOverview() {
    if (deferPresentation([this] { ToggleOverview(); }))
        return;
    if (!isActive())
        return;
    if (m_input)
        m_input->cancel();
    if (m_selecting) {
        ExitSplit();
        return;
    }
    if (!m_overview) {
        m_splitLeft.clear();
        m_splitRight.clear();
        refitAll();
    }
    m_overview = !m_overview;
    m_gesture.cancel();
    refreshLayout();
}
void WorkspaceEffect::BeginSplit(const QString &key, int side) {
    if (deferPresentation([this, key, side] { BeginSplit(key, side); }))
        return;
    auto w = find(key);
    if (!w || (side != -1 && side != 1))
        return;
    if (m_input)
        m_input->cancel();
    m_splitLeft = side < 0 ? w : nullptr;
    m_splitRight = side > 0 ? w : nullptr;
    refitAll();
    w->setMinimized(false);
    m_selecting = true;
    m_overview = true;
    m_gesture.cancel();
    refreshLayout();
}
void WorkspaceEffect::ChooseSplit(const QString &key) {
    if (deferPresentation([this, key] { ChooseSplit(key); }))
        return;
    auto w = find(key);
    if (!w || !m_selecting || w == m_splitLeft || w == m_splitRight)
        return;
    if (m_input)
        m_input->cancel();
    if (m_splitLeft)
        m_splitRight = w;
    else
        m_splitLeft = w;
    refitAll();
    w->setMinimized(false);
    workspace()->activateWindow(w, true);
    m_lastActive = w;
    m_selecting = false;
    m_overview = false;
    m_gesture.cancel();
    refreshLayout();
}
void WorkspaceEffect::ExitSplit() {
    if (deferPresentation([this] { ExitSplit(); }))
        return;
    if (m_input)
        m_input->cancel();
    m_splitLeft.clear();
    m_splitRight.clear();
    refitAll();
    m_selecting = false;
    m_overview = false;
    m_gesture.cancel();
    refreshLayout();
}
void WorkspaceEffect::ExitSplitFor(const QString &key) {
    if (deferPresentation([this, key] { ExitSplitFor(key); }))
        return;
    ExitSplit();
    Activate(key);
}
void WorkspaceEffect::overviewRelease(QPointF p) {
    const auto action = m_gesture.end(p);
    if (action.type == OverviewAction::Activate)
        Activate(action.id);
    if (action.type == OverviewAction::BeginSplit)
        BeginSplit(action.id, action.side);
}
QString WorkspaceEffect::StateJson() const {
    QJsonArray windows;
    for (auto w : workspace()->stackingOrder())
        if (m_windows.contains(w))
            windows.append(QJsonObject{{"id", id(w)},
                                       {"title", w->caption()},
                                       {"appId", appId(w)},
                                       {"circular", m_windows[w].circular},
                                       {"minimized", w->isMinimized()},
                                       {"active", w == active()},
                                       {"resizable", w->isResizable()},
                                       {"sourceGeometry", jsonRect(w->frameGeometry())},
                                       {"clientGeometry", jsonRect(w->clientGeometry())},
                                       {"bufferGeometry", jsonRect(w->bufferGeometry())},
                                       {"noBorder", w->noBorder()},
                                       {"requestedGeometry", jsonRect(sourceCanvas(w))},
                                       {"canvasAccepted", QRectF(w->frameGeometry()) == sourceCanvas(w)}});
    QJsonArray transients;
    for (auto window : workspace()->stackingOrder()) {
        auto root = rootFor(window);
        if (root && root != window) {
            transients.append(QJsonObject{{"id", id(window)},
                                          {"parentId", id(root)},
                                          {"title", window->caption()},
                                          {"popup", window->isPopupWindow()},
                                          {"sourceGeometry", jsonRect(window->frameGeometry())},
                                          {"bufferGeometry", jsonRect(window->bufferGeometry())}});
        }
    }
    QJsonArray placements;
    for (const auto &p : m_placements)
        placements.append(
            QJsonObject{{"id", p.id}, {"bounds", jsonRect(p.bounds)}, {"circular", p.circular}});
    return QString::fromUtf8(
        QJsonDocument(QJsonObject{{"enabled", m_enabled},
                                  {"captureActive", m_input && m_input->hasActiveCapture()},
                                  {"pendingActions", int(m_deferred.size())},
                                  {"ready", m_ready},
                                  {"overview", m_overview},
                                  {"activeId", id(active())},
                                  {"windows", windows},
                                  {"transients", transients},
                                  {"output", jsonRect(canvas())},
                                  {"content", jsonRect(content())},
                                  {"placements", placements},
                                  {"splitLeftId", id(m_splitLeft)},
                                  {"splitRightId", id(m_splitRight)},
                                  {"selectingPartner", m_selecting}})
            .toJson(QJsonDocument::Compact));
}
} // namespace LinuxRond
