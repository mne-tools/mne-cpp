#ifndef MNESCANPLUGINCREATOR_H
#define MNESCANPLUGINCREATOR_H

#include "plugincreator.h"

class MNEScanPluginCreator : public PluginCreator
{
public:
  MNEScanPluginCreator();

protected:
  QList<QPair<QString, QString>> templateInputOutputPairs(const PluginParams &params) override;
  void updateProjectFile(const PluginParams &params) override;

private:

  QString folderName(const QString &pluginName) const;

  const QString m_templatesPath;
  const QString m_pluginsPath;
  const QString m_profilePath;
};

#endif // MNESCANPLUGINCREATOR_H
