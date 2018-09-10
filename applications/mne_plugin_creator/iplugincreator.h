#ifndef PLUGINCREATOR_H
#define PLUGINCREATOR_H

#include <QRegularExpression>
#include <QDebug>

#include <stdexcept>

#include "pluginparams.h"
#include "templatefile.h"

class IPluginCreator {
public:
  IPluginCreator();
  void createPlugin(PluginParams &params);

protected:
  virtual QList<QPair<QString, QString>> templateInputOutputPairs(const PluginParams &params) const = 0;
  virtual void updateProjectFile(const PluginParams &params) const = 0;

private:
  void copyTemplates(const PluginParams &params) const;
};

#endif // PLUGINCREATOR_H
