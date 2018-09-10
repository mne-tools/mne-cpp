#include "plugincreator.h"

PluginCreator::PluginCreator() {}

void PluginCreator::createPlugin(PluginParams &params) {
  qDebug() << "Creating plugin: " << params.m_name << "..." << endl;
  copyTemplates();
  updateProjectFile(params);
  qDebug() << "Successfully created new " << params.m_name << " plugin!" << endl;
}

void PluginCreator::copyTemplates() const
{

}
