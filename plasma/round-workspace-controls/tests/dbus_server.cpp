// SPDX-License-Identifier: GPL-2.0-or-later
#include <QCoreApplication>
#include <QDBusConnection>
#include <QJsonDocument>
#include <QJsonObject>
class Backend final : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.softyoda.LinuxRond")
public Q_SLOTS:
    QString StateJson() const {
        return QString::fromUtf8(QJsonDocument(QJsonObject{{"enabled",enabled},{"overview",false},
            {"activeId","one"},{"splitLeftId","left-id"},{"splitRightId","right-id"},
            {"lastAction",lastAction}}).toJson(QJsonDocument::Compact));
    }
    void Minimize(const QString &id) { lastAction="minimize:"+id; }
    void SetEnabled(bool value) { enabled=value; }
private:
    bool enabled=true;
    QString lastAction;
};
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    Backend backend;
    auto bus=QDBusConnection::sessionBus();
    if (!bus.registerService("org.kde.KWin") || !bus.registerObject("/LinuxRond",&backend,QDBusConnection::ExportAllSlots)) return 1;
    return app.exec();
}
#include "dbus_server.moc"
