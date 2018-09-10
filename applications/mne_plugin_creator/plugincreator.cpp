#include "plugincreator.h"

PluginCreator::PluginCreator() : out(stdout) {}

void PluginCreator::createPlugin(PluginParams &params) {
  out << "Creating plugin: " << params.m_name << "..." << endl;
  copyTemplates(params);
  updateProjectFile(params);
  out << "Successfully created new " << params.m_name << " plugin!" << endl;
}
