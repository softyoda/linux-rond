// SPDX-License-Identifier: GPL-2.0-or-later
#include "workspace.h"
class WorkspaceFactory final : public KWin::EffectPluginFactory {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID EffectPluginFactory_iid FILE "metadata.json")
    Q_INTERFACES(KPluginFactory)
  public:
    bool isSupported() const override { return KWin::OffscreenEffect::supported(); }
    bool enabledByDefault() const override { return false; }
    KWin::Effect *createEffect() const override { return new LinuxRond::WorkspaceEffect; }
};
#include "main.moc"
