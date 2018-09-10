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
  PluginCreator();
  void createPlugin(PluginParams &params);

protected:
  virtual void copyTemplates(const PluginParams &params) = 0;
  virtual void updateProjectFile(const PluginParams &params) = 0;
  QTextStream out;
};

#endif // PLUGINCREATOR_H
