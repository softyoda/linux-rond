// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QPointF>
#include <QRectF>
#include <QVector>
#include <array>
#include <optional>

namespace LinuxRond {
enum class Shape { Square, Circle, LeftHalf, RightHalf, RoundedSquare };
struct WarpVertex {
    QPointF source;
    QPointF destination;
};
using WarpTriangle = std::array<WarpVertex, 3>;

class WarpMesh {
  public:
    static WarpMesh fromJson(const QByteArray &json, Shape shape);
    QPointF forward(QPointF source) const;
    std::optional<QPointF> inverse(QPointF destination, bool clamp = false) const;
    QVector<WarpTriangle> clipped(const QRectF &source) const;
    const QVector<WarpTriangle> &triangles() const { return m_triangles; }
    bool valid() const { return !m_triangles.isEmpty(); }

  private:
    int m_size = 0;
    QVector<WarpTriangle> m_triangles;
    QVector<std::array<WarpVertex, 2>> m_boundary;
    std::array<QVector<int>, 32 * 32> m_bins;
};
QPointF toNormalized(const QPointF &point, const QRectF &rect);
QPointF fromNormalized(const QPointF &point, const QRectF &rect);
} // namespace LinuxRond
