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
  void copyTemplates(QString pluginName);
  void copyFile(QString from, QString to);

  QString pluginsPath();
  QString srcPath(QString pluginName);
  QString formsPath(QString pluginName);
  QString imagesPath(QString pluginName);

  QTextStream out;
};

#endif // PLUGINCREATOR_H
