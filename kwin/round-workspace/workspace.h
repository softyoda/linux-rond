// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "geometry.h"
#include "input.h"
#include "overview.h"
#include "preferences.h"
#include <QHash>
#include <QPointer>
#include <QSettings>
#include <effect/effectwindow.h>
#include <effect/offscreeneffect.h>
#include <map>

namespace KWin {
class Window;
class LogicalOutput;
} // namespace KWin
namespace LinuxRond {
class WorkspaceEffect final : public KWin::OffscreenEffect {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.softyoda.LinuxRond")
  public:
    WorkspaceEffect();
    ~WorkspaceEffect() override;
    bool isActive() const override;
    int requestedEffectChainPosition() const override { return 90; }
    void prePaintScreen(KWin::ScreenPrePaintData &data) override;
    void prePaintWindow(KWin::RenderView *view, KWin::EffectWindow *window,
                        KWin::WindowPrePaintData &data) override;
    bool paintWindow(const KWin::RenderTarget &, const KWin::RenderViewport &, KWin::EffectWindow *, int,
                     const KWin::Region &, KWin::WindowPaintData &) override;
    bool blocksDirectScanout() const override { return isActive(); }
  public Q_SLOTS:
    Q_SCRIPTABLE QString StateJson() const;
    Q_SCRIPTABLE void SetEnabled(bool enabled);
    Q_SCRIPTABLE void ToggleOverview();
    Q_SCRIPTABLE void ToggleActiveMode();
    Q_SCRIPTABLE void MinimizeActive();
    Q_SCRIPTABLE void CloseActive();
    Q_SCRIPTABLE void ToggleMode(const QString &id);
    Q_SCRIPTABLE void Minimize(const QString &id);
    Q_SCRIPTABLE void Close(const QString &id);
    Q_SCRIPTABLE void Activate(const QString &id);
    Q_SCRIPTABLE void BeginSplit(const QString &id, int side);
    Q_SCRIPTABLE void ChooseSplit(const QString &id);
    Q_SCRIPTABLE void ExitSplit();
    Q_SCRIPTABLE void ExitSplitFor(const QString &id);
  Q_SIGNALS:
    void StateChanged(const QString &json);

  protected:
    void apply(KWin::EffectWindow *, int, KWin::WindowPaintData &, KWin::WindowQuadList &) override;

  private:
    struct SavedWindow {
        QPointer<KWin::Window> window;
        QRectF geometry;
        bool noBorder = false, fullscreen = false, circular = false;
        KWin::MaximizeMode maximize = KWin::MaximizeRestore;
    };
    struct Presentation {
        QRectF source, destination;
        Shape shape = Shape::Square;
    };
    struct CachedQuads {
        quint64 generation = 0;
        QRectF frame, expanded;
        KWin::WindowQuadList quads;
    };
    bool deferPresentation(std::function<void()> action);
    void drainDeferred();
    bool eligible(KWin::Window *) const;
    void observe(KWin::Window *);
    void manage(KWin::Window *);
    void fit(KWin::Window *, bool retry = false);
    void restore();
    void refreshOutput();
    void refreshLayout();
    void changed();
    KWin::Window *rootFor(KWin::Window *) const;
    KWin::Window *find(const QString &) const;
    KWin::Window *active() const;
    QString id(KWin::Window *) const;
    QString appId(KWin::Window *) const;
    QRectF canvas() const;
    QRectF sourceCanvas(KWin::Window *) const;
    void refitAll();
    QRectF content() const;
    std::optional<Presentation> presentation(KWin::Window *) const;
    std::optional<InputTarget> hitTest(QPointF) const;
    std::optional<QPointF> mapToSource(KWin::Window *, QPointF, bool clamp = true) const;
    const WarpMesh &mesh(Shape) const;
    void overviewRelease(QPointF);

    QSettings m_settings;
    ModePreferences m_preferences;
    std::array<WarpMesh, 5> m_meshes;
    std::unique_ptr<RoundInputFilter> m_input;
    QVector<std::function<void()>> m_deferred;
    QPointer<KWin::LogicalOutput> m_output;
    QHash<KWin::Window *, SavedWindow> m_windows;
    QSet<KWin::Window *> m_observed, m_pendingFit;
    QHash<KWin::Window *, int> m_fitAttempts;
    QSet<KWin::EffectWindow *> m_redirected;
    std::map<KWin::EffectWindow *, std::unique_ptr<KWin::EffectWindowVisibleRef>> m_visible;
    QHash<KWin::EffectWindow *, CachedQuads> m_quads;
    QPointer<KWin::Window> m_lastActive, m_splitLeft, m_splitRight;
    QVector<OverviewPlacement> m_placements;
    OverviewGesture m_gesture;
    bool m_enabled = false, m_overview = false, m_selecting = false, m_busy = false, m_ready = false;
    quint64 m_generation = 1;
};
} // namespace LinuxRond
