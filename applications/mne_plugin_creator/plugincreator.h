#ifndef PLUGINCREATOR_H
#define PLUGINCREATOR_H

#include <QRegularExpression>
#include <QDebug>

#include <stdexcept>

#include "pluginparams.h"
#include "templatefile.h"

class PluginCreator {
public:
  PluginCreator();
  void createPlugin(PluginParams &params);

protected:
  virtual QList<QPair<QString, QString>> templateInputOutputPairs(const PluginParams &params) const = 0;
  virtual void updateProjectFile(const PluginParams &params) const = 0;

private:
  void copyTemplates() const;
};

#endif // PLUGINCREATOR_H
