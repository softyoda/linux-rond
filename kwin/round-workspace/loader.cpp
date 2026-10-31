// SPDX-License-Identifier: GPL-2.0-or-later
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <dlfcn.h>
#include <plugin.h>

namespace {
void loaderLocation() {}
class LibraryPathPlugin final : public KWin::Plugin {
  public:
    explicit LibraryPathPlugin(QString path) : m_path(std::move(path)) {
        m_existing = QCoreApplication::libraryPaths().contains(m_path);
        QCoreApplication::addLibraryPath(m_path);
    }
    ~LibraryPathPlugin() override {
        if (!m_existing)
            QCoreApplication::removeLibraryPath(m_path);
    }

  private:
    QString m_path;
    bool m_existing = false;
};
} // namespace
class WorkspaceLoaderFactory final : public KWin::PluginFactory {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginFactory_iid FILE "loader-metadata.json")
    Q_INTERFACES(KWin::PluginFactory)
  public:
    std::unique_ptr<KWin::Plugin> create() const override {
        Dl_info info{};
        if (!dladdr(reinterpret_cast<void *>(&loaderLocation), &info) || !info.dli_fname)
            return {};
        const QString path = QFileInfo(QString::fromLocal8Bit(info.dli_fname)).absolutePath() + "/plugins";
        if (!QFileInfo::exists(path + "/kwin/effects/plugins/linux_rond_workspace.so"))
            return {};
        return std::make_unique<LibraryPathPlugin>(QDir(path).canonicalPath());
    }
};
#include "loader.moc"
