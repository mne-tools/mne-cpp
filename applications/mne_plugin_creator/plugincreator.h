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
  virtual QList<QPair<QString, QString>> templateInputOutputPairs(const PluginParams &params) = 0;
  virtual void updateProjectFile(const PluginParams &params) = 0;
  QTextStream out;

private:
  void copyTemplates(QList<TemplateFile> templates);
};

#endif // PLUGINCREATOR_H
