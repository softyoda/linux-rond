// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QSettings>
#include <QString>
namespace LinuxRond {
class ModePreferences {
  public:
    explicit ModePreferences(const QString &path) : settings(path, QSettings::IniFormat) {}
    bool circular(const QString &appId) const {
        return !appId.isEmpty() && settings.value(key(appId), false).toBool();
    }
    void setCircular(const QString &appId, bool value) {
        if (appId.isEmpty())
            return;
        settings.setValue(key(appId), value);
        settings.sync();
    }

  private:
    static QString key(const QString &appId) {
        return "Applications/" +
               QString::fromLatin1(
                   appId.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)) +
               "/circular";
    }
    QSettings settings;
};
} // namespace LinuxRond
