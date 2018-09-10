#ifndef MNESCANPLUGINCREATOR_H
#define MNESCANPLUGINCREATOR_H

#include "iplugincreator.h"

class MNEScanPluginCreator : public IPluginCreator
{
public:
  MNEScanPluginCreator();

protected:
  QList<QPair<QString, QString>> templateInputOutputPairs(const PluginParams &params) const override;
  void updateProjectFile(const PluginParams &params) const override;

private:

  QString folderName(const QString &pluginName) const;
  QString srcPath(const QString &pluginName) const;
  QString iconsPath(const QString &pluginName) const;
  QString formsPath(const QString &PluginName) const;

  const QString m_templatesPath;
  const QString m_pluginsPath;
  const QString m_profilePath;
};

#endif // MNESCANPLUGINCREATOR_H
