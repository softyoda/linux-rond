// SPDX-License-Identifier: GPL-2.0-or-later
#include "overview.h"
#include <QDebug>
#include <QPainterPath>
#include <cmath>
#include <cstdlib>
using namespace LinuxRond;
static constexpr qreal Pi = 3.14159265358979323846;
static void check(bool ok, const char *message) {
    if (!ok) { qCritical() << message; std::abort(); }
}
static QPainterPath outline(const OverviewPlacement &p) {
    QPainterPath path;
    if (p.circular) path.addEllipse(p.bounds);
    else path.addRoundedRect(p.bounds,p.cornerRadius,p.cornerRadius);
    return path;
}
static qreal area(const OverviewPlacement &p) {
    return p.circular ? Pi*p.bounds.width()*p.bounds.height()/4
        : p.bounds.width()*p.bounds.height()-(4-Pi)*p.cornerRadius*p.cornerRadius;
}
int main() {
    const QRectF output(150, 90, 720, 720);
    for (int count : {1,2,3,6,9,19,48,100}) {
        QVector<OverviewWindow> windows;
        for (int i = 0; i < count; ++i) windows.push_back({QString::number(i), i % 2 == 0, i % 3 == 0, i == 1});
        for (auto region : {OverviewRegion::Full,OverviewRegion::Left,OverviewRegion::Right}) {
            auto p = packOverview(windows, output, region);
            check(p.size() == count, "Missing previews");
            for (int i = 0; i < count; ++i) {
                const auto &a = p[i];
                const qreal half=a.bounds.width()/2;
                const qreal corner=a.circular?half:a.cornerRadius;
                const qreal base=half-corner;
                auto delta = a.bounds.center() - output.center();
                check(std::hypot(std::abs(delta.x())+base,std::abs(delta.y())+base)+corner <= 348.0001, "Preview outside disk");
                if (region == OverviewRegion::Left) check(a.bounds.right() < output.center().x(), "Crosses right half");
                if (region == OverviewRegion::Right) check(a.bounds.left() > output.center().x(), "Crosses left half");
                check(hitTestOverview(p,a.bounds.center()) == a.id, "Wrong hit target");
                for (int j = 0; j < i; ++j) check(!outline(a).intersects(outline(p[j])), "Overlapping previews");
            }
        }
    }
    for (auto [count,minimum] : {std::pair{1,.95}, {2,.43}, {3,.55}, {6,.52}, {9,.52}}) {
        QVector<OverviewWindow> windows;
        for(int i=0;i<count;++i)windows.push_back({QString::number(i),true,false,false});
        qreal total=0;
        for(const auto &p:packOverview(windows,output))total+=area(p);
        const qreal occupancy=total/(Pi*348*348);
        qInfo()<<"count"<<count<<"occupancy"<<occupancy;
        check(occupancy>=minimum,"Packing leaves avoidable empty area");
    }
    auto sizes = packOverview({{"open",true,false,false},{"min",true,true,false}}, output);
    check(area(sizes[0])>area(sizes[1]), "Minimized preview not smaller");
    check(hitTestOverview(sizes,sizes[0].bounds.topLeft()).isEmpty(), "Circle corner accepted");
    auto shapes = packOverview({{"circle",true,false,false},{"square",false,false,false}}, output);
    check(std::abs(area(shapes[0])-area(shapes[1]))<1e-5, "Equal-state shapes have different area");
    OverviewGesture gesture;
    check(gesture.begin(sizes[0].bounds.center(),sizes,output), "Cannot start tap");
    check(gesture.end(sizes[0].bounds.center()).type==OverviewAction::Activate, "Tap did not activate");
    gesture.begin(sizes[0].bounds.center(),sizes,output);
    auto action=gesture.end(QPointF(output.left()+40,output.center().y()));
    check(action.type==OverviewAction::BeginSplit && action.side==-1, "Left drag did not split");
    gesture.begin(sizes[0].bounds.center(),sizes,output);
    action=gesture.end(QPointF(output.right()-40,output.center().y()));
    check(action.type==OverviewAction::BeginSplit && action.side==1, "Right drag did not split");
    gesture.begin(sizes[0].bounds.center(),sizes,output);
    gesture.cancel();
    check(gesture.end(output.center()).type==OverviewAction::None, "Cancelled gesture activated");
    check(packOverview({},output).isEmpty(), "Empty list produced previews");
    qInfo()<<"Overview layout and gesture tests passed";
}
