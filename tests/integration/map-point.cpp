// SPDX-License-Identifier: GPL-2.0-or-later
#include "geometry.h"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);const auto a=app.arguments();
    if(a.size()!=9)return 2;
    QFile file(a[1]);if(!file.open(QIODevice::ReadOnly))return 1;
    auto mesh=LinuxRond::WarpMesh::fromJson(file.readAll(),static_cast<LinuxRond::Shape>(a[2].toInt()));
    if(!mesh.valid())return 1;
    const auto point=LinuxRond::fromNormalized(mesh.forward({a[3].toDouble(),a[4].toDouble()}),
        QRectF(a[5].toDouble(),a[6].toDouble(),a[7].toDouble(),a[8].toDouble()));
    QTextStream out(stdout);out.setRealNumberPrecision(14);out<<point.x()<<' '<<point.y()<<'\n';
}
