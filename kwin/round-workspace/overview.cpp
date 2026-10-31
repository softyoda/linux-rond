// SPDX-License-Identifier: GPL-2.0-or-later
#include "overview.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace LinuxRond {
namespace {
constexpr qreal Pi = 3.14159265358979323846;
constexpr qreal CornerFraction = .10;
struct Footprint {
    qreal base;
    qreal corner;
    qreal half() const { return base + corner; }
};
qreal weight(const OverviewWindow &window) {
    return window.minimized ? .82 : (window.active ? 1.08 : 1.0);
}
Footprint footprint(const OverviewWindow &window, qreal scale) {
    const qreal radius = weight(window) * scale;
    if (window.circular) return {0, radius};
    const qreal side = radius * std::sqrt(Pi / (1 - (4 - Pi) * CornerFraction * CornerFraction));
    return {side * (.5 - CornerFraction), side * CornerFraction};
}
qreal sign(qreal value) { return value < 0 ? -1 : 1; }
qreal separation(const QPointF &delta, qreal base, qreal corner, QPointF *normal = nullptr) {
    const QPointF q(std::abs(delta.x()) - base, std::abs(delta.y()) - base);
    const QPointF positive(std::max(q.x(), 0.), std::max(q.y(), 0.));
    const qreal length = std::hypot(positive.x(), positive.y());
    if (normal) {
        if (length > 1e-12) *normal = {sign(delta.x()) * positive.x() / length, sign(delta.y()) * positive.y() / length};
        else if (q.x() > q.y()) *normal = {sign(delta.x()), 0};
        else *normal = {0, sign(delta.y())};
    }
    return length + std::min(std::max(q.x(), q.y()), 0.) - corner;
}
qreal boundaryError(const QPointF &point, const Footprint &shape, OverviewRegion region, qreal gap) {
    qreal error = std::hypot(std::abs(point.x()) + shape.base, std::abs(point.y()) + shape.base) + shape.corner - 1;
    if (region == OverviewRegion::Left) error = std::max(error, point.x() + shape.half() + gap / 2);
    if (region == OverviewRegion::Right) error = std::max(error, -point.x() + shape.half() + gap / 2);
    return error;
}
void confine(QPointF &point, const Footprint &shape, OverviewRegion region, qreal gap) {
    for (int iteration = 0; iteration < 3; ++iteration) {
        if (region == OverviewRegion::Left) point.setX(std::min(point.x(), -shape.half() - gap / 2));
        if (region == OverviewRegion::Right) point.setX(std::max(point.x(), shape.half() + gap / 2));
        if (std::hypot(std::abs(point.x()) + shape.base, std::abs(point.y()) + shape.base) + shape.corner <= 1) continue;
        const qreal a = point.x() * point.x() + point.y() * point.y();
        const qreal b = 2 * shape.base * (std::abs(point.x()) + std::abs(point.y()));
        const qreal c = 2 * shape.base * shape.base - (1 - shape.corner) * (1 - shape.corner);
        if (a < 1e-12) continue;
        const qreal t = (-b + std::sqrt(std::max(0., b * b - 4 * a * c))) / (2 * a);
        point *= std::clamp(t, 0., 1.);
    }
}
qreal violation(const QVector<QPointF> &points, const QVector<Footprint> &shapes, OverviewRegion region, qreal gap) {
    qreal error = 0;
    for (qsizetype i = 0; i < points.size(); ++i) {
        error = std::max(error, boundaryError(points[i], shapes[i], region, gap));
        for (qsizetype j = 0; j < i; ++j) {
            error = std::max(error, gap - separation(points[i] - points[j], shapes[i].base + shapes[j].base,
                                                     shapes[i].corner + shapes[j].corner));
        }
    }
    return error;
}
bool relax(QVector<QPointF> &points, const QVector<Footprint> &shapes, OverviewRegion region, qreal gap, int iterations) {
    for (int iteration = 0; iteration < iterations; ++iteration) {
        for (qsizetype i = 0; i < points.size(); ++i) {
            for (qsizetype j = 0; j < i; ++j) {
                QPointF normal;
                const qreal depth = gap - separation(points[i] - points[j], shapes[i].base + shapes[j].base,
                                                       shapes[i].corner + shapes[j].corner, &normal);
                if (depth > 0) {
                    points[i] += normal * (depth * .51);
                    points[j] -= normal * (depth * .51);
                }
            }
        }
        for (qsizetype i = 0; i < points.size(); ++i) confine(points[i], shapes[i], region, gap);
        if (iteration % 10 == 0 && violation(points, shapes, region, gap) < 1e-7) return true;
    }
    return violation(points, shapes, region, gap) < 1e-7;
}
QVector<QPointF> seed(qsizetype count, int pattern, OverviewRegion region) {
    QVector<QPointF> points;
    for (qsizetype i = 0; i < count; ++i) {
        qreal angle = 0, radius = 0;
        if (count > 1) {
            if (pattern % 3 == 0) { angle = 2 * Pi * i / count; radius = .6; }
            else if (pattern % 3 == 1) {
                angle = 2 * Pi * std::max<qsizetype>(0, i - 1) / std::max<qsizetype>(1, count - 1);
                radius = i == 0 ? 0 : .65;
            } else { angle = i * 2.399963229728653; radius = .75 * std::sqrt((i + .5) / count); }
        }
        angle += pattern * .31;
        QPointF point(radius * std::cos(angle), radius * std::sin(angle));
        if (region != OverviewRegion::Full) point.setX((region == OverviewRegion::Left ? -1 : 1) * (.25 + .65 * std::abs(point.x())));
        points.push_back(point);
    }
    return points;
}
bool contains(const OverviewPlacement &placement, const QPointF &point) {
    if (!placement.bounds.contains(point)) return false;
    const qreal half = placement.bounds.width() / 2;
    const qreal corner = placement.circular ? half : placement.cornerRadius;
    return separation(point - placement.bounds.center(), half - corner, corner) <= 0;
}
}

QVector<OverviewPlacement> packOverview(const QVector<OverviewWindow> &windows,
    const QRectF &output, OverviewRegion region, qreal margin, qreal gap) {
    if (windows.isEmpty() || output.width() <= 0 || output.height() <= 0) return {};
    const qreal diskRadius = std::min(output.width(), output.height()) / 2 - std::max(0., margin);
    if (diskRadius <= 0) return {};
    QVector<OverviewWindow> ordered = windows;
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) {
        if (weight(a) != weight(b)) return weight(a) > weight(b);
        return a.id < b.id;
    });
    gap = std::clamp(gap / diskRadius, 0., .3 / std::sqrt(ordered.size()));
    qreal areaWeight = 0;
    for (const auto &window : ordered) areaWeight += weight(window) * weight(window);
    qreal low = 0, high = std::sqrt((region == OverviewRegion::Full ? 1. : .5) / areaWeight);
    QVector<QPointF> best;
    const int attempts = ordered.size() <= 12 ? 6 : (ordered.size() <= 32 ? 4 : 2);
    const int iterations = ordered.size() <= 32 ? 450 : 220;
    for (int search = 0; search < 17; ++search) {
        const qreal scale = (low + high) / 2;
        QVector<Footprint> shapes;
        for (const auto &window : ordered) shapes.push_back(footprint(window, scale));
        bool found = false;
        for (int attempt = 0; attempt < attempts; ++attempt) {
            auto points = attempt == 0 && !best.isEmpty() ? best : seed(ordered.size(), attempt, region);
            if (relax(points, shapes, region, gap, iterations)) {
                low = scale;
                best = std::move(points);
                found = true;
                break;
            }
        }
        if (!found) high = scale;
    }
    if (best.size() != ordered.size()) return {};
    QVector<OverviewPlacement> result;
    result.reserve(ordered.size());
    // Leave a subpixel safety margin after numerical relaxation.
    for (qsizetype i = 0; i < ordered.size(); ++i) {
        const auto shape = footprint(ordered[i], low * .99999);
        const qreal side = 2 * shape.half() * diskRadius;
        const QPointF center = output.center() + best[i] * diskRadius;
        result.push_back({ordered[i].id, QRectF(center.x() - side / 2, center.y() - side / 2, side, side),
                          ordered[i].circular, ordered[i].circular ? 0 : shape.corner * diskRadius});
    }
    return result;
}

QString hitTestOverview(const QVector<OverviewPlacement> &placements, const QPointF &point) {
    for (auto it = placements.crbegin(); it != placements.crend(); ++it) {
        if (contains(*it, point)) return it->id;
    }
    return {};
}

bool OverviewGesture::begin(const QPointF &position, const QVector<OverviewPlacement> &placements, const QRectF &output) {
    cancel();
    m_id = hitTestOverview(placements, position);
    m_start = m_position = position;
    m_output = output;
    return !m_id.isEmpty();
}
void OverviewGesture::update(const QPointF &position) {
    if (m_id.isEmpty()) return;
    m_position = position;
    if (std::hypot(position.x() - m_start.x(), position.y() - m_start.y()) >= 18) m_dragging = true;
}
int OverviewGesture::dropSide() const {
    if (!m_dragging || !m_output.contains(m_position)) return 0;
    const QPointF delta = m_position - m_output.center();
    if (std::hypot(delta.x(), delta.y()) > std::min(m_output.width(), m_output.height()) / 2) return 0;
    const qreal threshold = .20 * m_output.width();
    const qreal offset = m_position.x() - m_output.center().x();
    return offset < -threshold ? -1 : (offset > threshold ? 1 : 0);
}
OverviewAction OverviewGesture::end(const QPointF &position) {
    update(position);
    OverviewAction action;
    if (!m_id.isEmpty()) {
        const int side = dropSide();
        if (m_dragging && side) action = {OverviewAction::BeginSplit, m_id, side};
        else if (!m_dragging) action = {OverviewAction::Activate, m_id, 0};
    }
    cancel();
    return action;
}
void OverviewGesture::cancel() {
    m_id.clear();
    m_dragging = false;
    m_output = {};
}
}
