#include "iplugincreator.h"

IPluginCreator::IPluginCreator() {}

void IPluginCreator::createPlugin(PluginParams &params) {
  qDebug() << "Creating plugin: " << params.m_name << "..." << endl;
  copyTemplates(params);
  updateProjectFile(params);
  qDebug() << "Successfully created new " << params.m_name << " plugin!" << endl;
}

void IPluginCreator::copyTemplates(const PluginParams &params) const {
  for (QPair<QString, QString> pair : templateInputOutputPairs(params)) {
      TemplateFile templateFile(pair.first, pair.second);
      templateFile.fill(params);
  }
}
