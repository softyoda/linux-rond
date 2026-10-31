// SPDX-License-Identifier: GPL-2.0-or-later
#include "geometry.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace LinuxRond {
namespace {
double cross(QPointF a, QPointF b) { return a.x() * b.y() - a.y() * b.x(); }
double dot(QPointF a, QPointF b) { return a.x() * b.x() + a.y() * b.y(); }
std::array<double, 3> barycentric(QPointF point, QPointF a, QPointF b, QPointF c) {
    const double det = cross(b - a, c - a);
    const double u = cross(point - a, c - a) / det;
    const double v = cross(b - a, point - a) / det;
    return {1 - u - v, u, v};
}
QPointF interpolate(const WarpTriangle &triangle, const std::array<double, 3> &w, bool source) {
    QPointF result;
    for (int i = 0; i < 3; ++i)
        result += (source ? triangle[i].source : triangle[i].destination) * w[i];
    return result;
}
int bin(double x) { return std::clamp(int((x + 1) * 16), 0, 31); }
QPointF halfDisk(QPointF p) {
    const double s = (p.x() + 1) / 2, t = (p.y() + 1) / 2;
    const double pi = std::acos(-1.0), q = std::sqrt(.5);
    const QPointF left(0, -1 + 2 * t);
    const QPointF right(std::cos(-pi / 4 + pi / 2 * t), std::sin(-pi / 4 + pi / 2 * t));
    const QPointF top(std::cos(-pi / 2 + pi / 4 * s), std::sin(-pi / 2 + pi / 4 * s));
    const QPointF bottom(std::cos(pi / 2 - pi / 4 * s), std::sin(pi / 2 - pi / 4 * s));
    const QPointF corners = QPointF(0, -1) * (1 - s) * (1 - t) + QPointF(q, -q) * s * (1 - t) +
                            QPointF(0, 1) * (1 - s) * t + QPointF(q, q) * s * t;
    return left * (1 - s) + right * s + top * (1 - t) + bottom * t - corners;
}
QPointF roundedSquare(QPointF p) {
    constexpr double r = .20;
    if (std::abs(p.x()) <= 1 - r || std::abs(p.y()) <= 1 - r)
        return p;
    const double a = (std::abs(p.x()) - (1 - r)) / r, b = (std::abs(p.y()) - (1 - r)) / r;
    return {std::copysign(1 - r + r * a * std::sqrt(1 - b * b / 2), p.x()),
            std::copysign(1 - r + r * b * std::sqrt(1 - a * a / 2), p.y())};
}
} // namespace
QPointF toNormalized(const QPointF &p, const QRectF &r) {
    return {(p.x() - r.x()) * 2 / r.width() - 1, (p.y() - r.y()) * 2 / r.height() - 1};
}
QPointF fromNormalized(const QPointF &p, const QRectF &r) {
    return {r.x() + (p.x() + 1) * r.width() / 2, r.y() + (p.y() + 1) * r.height() / 2};
}

WarpMesh WarpMesh::fromJson(const QByteArray &json, Shape shape) {
    WarpMesh mesh;
    const auto object = QJsonDocument::fromJson(json).object();
    const int n = object.value("size").toInt();
    const auto rows = object.value("vertices").toArray();
    if (n < 3 || n > 257 || rows.size() != n)
        throw std::runtime_error("Invalid projection mesh");
    mesh.m_size = n;
    QVector<WarpVertex> vertices;
    for (int j = 0; j < n; ++j) {
        const auto row = rows[j].toArray();
        if (row.size() != n)
            throw std::runtime_error("Invalid mesh row");
        for (int i = 0; i < n; ++i) {
            const QPointF source(-1 + 2. * i / (n - 1), -1 + 2. * j / (n - 1));
            const auto values = row[i].toArray();
            if (values.size() != 2)
                throw std::runtime_error("Invalid mesh vertex");
            QPointF destination(values[0].toDouble(), values[1].toDouble());
            if (shape == Shape::Square)
                destination = source;
            if (shape == Shape::RoundedSquare)
                destination = roundedSquare(source);
            if (shape == Shape::RightHalf)
                destination = halfDisk(source);
            if (shape == Shape::LeftHalf) {
                destination = halfDisk({-source.x(), source.y()});
                destination.setX(-destination.x());
            }
            if (!std::isfinite(destination.x()) || !std::isfinite(destination.y()))
                throw std::runtime_error("Nonfinite mesh");
            vertices.push_back({source, destination});
        }
    }
    for (int j = 0; j < n - 1; ++j)
        for (int i = 0; i < n - 1; ++i) {
            const auto a = vertices[j * n + i], b = vertices[j * n + i + 1],
                       c = vertices[(j + 1) * n + i + 1], d = vertices[(j + 1) * n + i];
            for (const WarpTriangle &triangle : {WarpTriangle{a, b, c}, WarpTriangle{a, c, d}}) {
                if (cross(triangle[1].destination - triangle[0].destination,
                          triangle[2].destination - triangle[0].destination) <= 1e-12)
                    throw std::runtime_error("Folded projection mesh");
                const int index = mesh.m_triangles.size();
                mesh.m_triangles.push_back(triangle);
                double x0 = 1, y0 = 1, x1 = -1, y1 = -1;
                for (const auto &v : triangle) {
                    x0 = std::min(x0, v.destination.x());
                    x1 = std::max(x1, v.destination.x());
                    y0 = std::min(y0, v.destination.y());
                    y1 = std::max(y1, v.destination.y());
                }
                for (int y = bin(y0); y <= bin(y1); ++y)
                    for (int x = bin(x0); x <= bin(x1); ++x)
                        mesh.m_bins[y * 32 + x].push_back(index);
            }
        }
    for (int k = 0; k < n - 1; ++k) {
        mesh.m_boundary.push_back({vertices[k], vertices[k + 1]});
        mesh.m_boundary.push_back({vertices[(n - 1) * n + k], vertices[(n - 1) * n + k + 1]});
        mesh.m_boundary.push_back({vertices[k * n], vertices[(k + 1) * n]});
        mesh.m_boundary.push_back({vertices[k * n + n - 1], vertices[(k + 1) * n + n - 1]});
    }
    return mesh;
}
QPointF WarpMesh::forward(QPointF p) const {
    p.setX(std::clamp(p.x(), -1., 1.));
    p.setY(std::clamp(p.y(), -1., 1.));
    const double x = (p.x() + 1) * (m_size - 1) / 2, y = (p.y() + 1) * (m_size - 1) / 2;
    const int i = std::min(int(x), m_size - 2), j = std::min(int(y), m_size - 2);
    const int which = (y - j <= x - i) ? 0 : 1;
    const auto &triangle = m_triangles[(j * (m_size - 1) + i) * 2 + which];
    return interpolate(triangle, barycentric(p, triangle[0].source, triangle[1].source, triangle[2].source),
                       false);
}
std::optional<QPointF> WarpMesh::inverse(QPointF p, bool clamp) const {
    if (std::abs(p.x()) <= 1.000001 && std::abs(p.y()) <= 1.000001) {
        for (int index : m_bins[bin(p.y()) * 32 + bin(p.x())]) {
            const auto &t = m_triangles[index];
            const auto w = barycentric(p, t[0].destination, t[1].destination, t[2].destination);
            if (w[0] >= -1e-8 && w[1] >= -1e-8 && w[2] >= -1e-8)
                return interpolate(t, w, true);
        }
    }
    if (!clamp)
        return std::nullopt;
    double best = std::numeric_limits<double>::infinity();
    QPointF result;
    for (const auto &edge : m_boundary) {
        const auto d = edge[1].destination - edge[0].destination;
        const double t = std::clamp(dot(p - edge[0].destination, d) / dot(d, d), 0., 1.);
        const auto delta = p - (edge[0].destination + d * t);
        if (dot(delta, delta) < best) {
            best = dot(delta, delta);
            result = edge[0].source * (1 - t) + edge[1].source * t;
        }
    }
    return result;
}
QVector<WarpTriangle> WarpMesh::clipped(const QRectF &rect) const {
    QVector<WarpTriangle> result;
    for (const auto &triangle : m_triangles) {
        QVector<WarpVertex> polygon{triangle[0], triangle[1], triangle[2]};
        for (int edge = 0; edge < 4 && !polygon.empty(); ++edge) {
            const auto distance = [&](const QPointF &p) {
                if (edge == 0)
                    return p.x() - rect.left();
                if (edge == 1)
                    return rect.right() - p.x();
                if (edge == 2)
                    return p.y() - rect.top();
                return rect.bottom() - p.y();
            };
            QVector<WarpVertex> output;
            auto previous = polygon.back();
            double previousD = distance(previous.source);
            for (const auto &current : polygon) {
                const double currentD = distance(current.source);
                if ((currentD >= 0) != (previousD >= 0)) {
                    const double t = previousD / (previousD - currentD);
                    output.push_back({previous.source * (1 - t) + current.source * t,
                                      previous.destination * (1 - t) + current.destination * t});
                }
                if (currentD >= 0)
                    output.push_back(current);
                previous = current;
                previousD = currentD;
            }
            polygon = output;
        }
        for (int i = 1; i + 1 < polygon.size(); ++i) {
            if (cross(polygon[i].source - polygon[0].source, polygon[i + 1].source - polygon[0].source) >
                1e-14)
                result.push_back({polygon[0], polygon[i], polygon[i + 1]});
        }
    }
    return result;
}
} // namespace LinuxRond
