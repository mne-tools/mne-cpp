#ifndef PLUGINCREATOR_H
#define PLUGINCREATOR_H

#include <QDir>
#include <QRegularExpression>
#include <QTextStream>

#include <stdexcept>

#include "pluginparams.h"
#include "templatefile.h"

class PluginCreator {
public:
  explicit PluginCreator();
  void createPlugin(PluginParams &params);

protected:
  void createFolderStructure(QString pluginName);
  void createDirectory(QString path);
  void copyTemplates(PluginParams &params);
  void copyFile(QString from, QString to);
  void fillTemplates(PluginParams &params);
  void updateProjectFile(PluginParams &params);

  QString pluginsPath();
  QString folderName(QString pluginName);
  QString srcPath(QString pluginName);
  QString formsPath(QString pluginName);
  QString imagesPath(QString pluginName);
  QString iconsPath(QString pluginName);
  QString templatesPath();
  QString projectFileFilepath();

  QTextStream out;
};

#endif // PLUGINCREATOR_H
