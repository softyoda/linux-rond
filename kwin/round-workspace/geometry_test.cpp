// SPDX-License-Identifier: GPL-2.0-or-later
#include "geometry.h"
#include "preferences.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace LinuxRond;
void check(bool condition) {
    if (!condition)
        std::abort();
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QFile file(":/linux-rond/approved-mesh.json");
    check(file.open(QIODevice::ReadOnly));
    const auto json = file.readAll();
    for (auto shape :
         {Shape::Square, Shape::Circle, Shape::LeftHalf, Shape::RightHalf, Shape::RoundedSquare}) {
        const auto mesh = WarpMesh::fromJson(json, shape);
        check(mesh.valid());
        for (int j = 0; j <= 100; ++j)
            for (int i = 0; i <= 100; ++i) {
                const QPointF p(-1 + i * .02, -1 + j * .02);
                const auto q = mesh.forward(p);
                const auto back = mesh.inverse(q);
                check(back.has_value());
                check(std::hypot(back->x() - p.x(), back->y() - p.y()) < 1e-6);
                if (shape == Shape::Square)
                    check(std::hypot(q.x() - p.x(), q.y() - p.y()) < 1e-12);
                if (shape != Shape::Square && shape != Shape::RoundedSquare)
                    check(q.x() * q.x() + q.y() * q.y() <= 1 + 1e-8);
                if (shape == Shape::LeftHalf)
                    check(q.x() < 1e-8);
                if (shape == Shape::RightHalf)
                    check(q.x() > -1e-8);
            }
        for (const QPointF corner : {QPointF(-1, -1), QPointF(1, -1), QPointF(1, 1), QPointF(-1, 1)}) {
            if (shape == Shape::Square)
                check(mesh.forward(corner) == corner);
            if (shape == Shape::RoundedSquare)
                check(mesh.forward(corner) != corner);
        }
        check(!mesh.inverse({3, 3}).has_value());
        check(mesh.inverse({3, 3}, true).has_value());
        const QRectF clip(-.4, -.3, .7, .6);
        double clippedArea = 0;
        for (const auto &triangle : mesh.clipped(clip)) {
            const auto a = triangle[1].source - triangle[0].source;
            const auto b = triangle[2].source - triangle[0].source;
            clippedArea += (a.x() * b.y() - a.y() * b.x()) / 2;
            for (const auto &v : triangle) {
                check(v.source.x() >= clip.left() - 1e-9 && v.source.x() <= clip.right() + 1e-9);
                check(v.source.y() >= clip.top() - 1e-9 && v.source.y() <= clip.bottom() + 1e-9);
                const auto direct = mesh.forward(v.source);
                check(std::hypot(direct.x() - v.destination.x(), direct.y() - v.destination.y()) < 1e-6);
            }
        }
        check(std::abs(clippedArea - clip.width() * clip.height()) < 1e-9);
    }
    QTemporaryDir temporary;
    {
        ModePreferences prefs(temporary.filePath("prefs.ini"));
        check(!prefs.circular("app/test"));
        prefs.setCircular("app/test", true);
        prefs.setCircular("app2", false);
    }
    {
        ModePreferences prefs(temporary.filePath("prefs.ini"));
        check(prefs.circular("app/test"));
        check(!prefs.circular("app2"));
    }
    std::cout << "Projection round trips, clipping, half-disks, and persistent modes passed.\n";
}
