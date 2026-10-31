// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

namespace LinuxRond {
struct OverviewWindow {
    QString id;
    bool circular = false;
    bool minimized = false;
    bool active = false;
};
struct OverviewPlacement {
    QString id;
    QRectF bounds;
    bool circular = false;
    qreal cornerRadius = 0;
};
enum class OverviewRegion { Full, Left, Right };
QVector<OverviewPlacement> packOverview(const QVector<OverviewWindow> &windows,
    const QRectF &output, OverviewRegion region = OverviewRegion::Full,
    qreal margin = 12, qreal gap = 8);
QString hitTestOverview(const QVector<OverviewPlacement> &placements, const QPointF &point);

struct OverviewAction {
    enum Type { None, Activate, BeginSplit };
    Type type = None;
    QString id;
    int side = 0;
};
class OverviewGesture {
public:
    bool begin(const QPointF &position, const QVector<OverviewPlacement> &placements, const QRectF &output);
    void update(const QPointF &position);
    OverviewAction end(const QPointF &position);
    void cancel();
    bool dragging() const { return m_dragging; }
    QString windowId() const { return m_id; }
    QPointF position() const { return m_position; }
    int dropSide() const;
private:
    QString m_id;
    QPointF m_start;
    QPointF m_position;
    QRectF m_output;
    bool m_dragging = false;
};
}
