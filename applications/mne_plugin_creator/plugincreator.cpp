#include "plugincreator.h"

PluginCreator::PluginCreator(QObject *parent) : QObject(parent), out(stdout) {}

void PluginCreator::createPlugin(PluginParams &params) {
  out << QObject::tr("Creating plugin: ") << params.m_name << QObject::tr("...")
      << endl;
  createFolderStructure(params.m_name);
}

void PluginCreator::createFolderStructure(QString &pluginName) {
  QDir pluginsDir("../mne_scan/plugins");
  if (!pluginsDir.exists()) {
    throw std::runtime_error(
        "Could not find mne_scan plugins folder. Make sure you ran the plugin "
        "creator from within the mne_plugin_creator folder");
  }

  throw std::runtime_error("This is a test!");
}
