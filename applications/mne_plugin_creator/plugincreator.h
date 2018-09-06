#ifndef PLUGINCREATOR_H
#define PLUGINCREATOR_H

#include <QDir>
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
  void createFolderStructure(QString &pluginName);

  QTextStream out;
};

#endif // PLUGINCREATOR_H
