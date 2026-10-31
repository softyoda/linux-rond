// SPDX-License-Identifier: GPL-2.0-or-later
#include <QCoreApplication>
#include <QDBusConnection>
#include <QTimer>

class DemoControl : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.softyoda.LinuxRondDemo")
public Q_SLOTS:
    void Quit() { QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit); }
};
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    if(qEnvironmentVariable("LINUX_ROND_DEMO_SESSION")!="1")return 2;
    DemoControl control;
    auto bus=QDBusConnection::sessionBus();
    if(!bus.registerService("org.softyoda.LinuxRondDemo")||
       !bus.registerObject("/Demo",&control,QDBusConnection::ExportAllSlots))return 1;
    return app.exec();
}
#include "demo-control.moc"
