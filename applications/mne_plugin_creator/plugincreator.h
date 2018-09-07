#ifndef PLUGINCREATOR_H
#define PLUGINCREATOR_H

#include <QDir>
#include <QFile>
#include <QObject>
#include <QTextStream>

#include <stdexcept>

#include "pluginparams.h"

class PluginCreator : public QObject {
  Q_OBJECT
public:
  explicit PluginCreator(QObject *parent);
  void createPlugin(PluginParams &params);

protected:
  void createFolderStructure(QString pluginName);
  void createDirectory(QString path);
  void copyTemplates(PluginParams &params);
  void copyFile(QString from, QString to);
  void fillTemplates(PluginParams &params);
  void fillSingleTemplate(QFile &file, PluginParams &params);

  QString pluginsPath();
  QString srcPath(QString pluginName);
  QString formsPath(QString pluginName);
  QString imagesPath(QString pluginName);
  QString iconsPath(QString pluginName);
  QString templatesPath();

  QTextStream out;
};

#endif // PLUGINCREATOR_H
