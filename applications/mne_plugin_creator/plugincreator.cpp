#include "plugincreator.h"

PluginCreator::PluginCreator(QObject *parent) : QObject(parent), out(stdout) {}

void PluginCreator::createPlugin(PluginParams &params) {
  out << "Creating plugin: " << params.m_name << "..." << endl;
  createFolderStructure(params.m_name);
  copyTemplates(params.m_name);
}

QString PluginCreator::pluginsPath() {
  return "../../../mne-cpp/applications/mne_scan/plugins/";
}

QString PluginCreator::srcPath(QString pluginName) {
  return pluginsPath() + pluginName.toLower() + "/";
}

QString PluginCreator::formsPath(QString pluginName) {
  return srcPath(pluginName) + "FormFiles/";
}

QString PluginCreator::imagesPath(QString pluginName) {
  return srcPath(pluginName) + "images/";
}

QString PluginCreator::templatesPath(){
  return "../../../mne-cpp/applications/mne_plugin_creator/templates/";
}

void PluginCreator::createFolderStructure(QString pluginName) {
  out << "Attempting to create folder structure for your new plugin at "
      << QDir(srcPath(pluginName)).absolutePath() << "..." << endl;

  if (!QDir(pluginsPath()).exists()) {
    throw std::runtime_error(
        "Could not find mne_scan plugins folder. Make sure you ran the plugin "
        "creator from within the mne_plugin_creator folder");
  }

  createDirectory(srcPath(pluginName));
  out << "Created source folder at " << srcPath(pluginName) << endl;

  createDirectory(formsPath(pluginName));
  out << "Created form files folder at " << formsPath(pluginName) << endl;

  createDirectory(imagesPath(pluginName));
  out << "Created images folder at " << imagesPath(pluginName) << endl;
}

void PluginCreator::createDirectory(QString path) {
  bool success = QDir::current().mkdir(path);
  if (!success) {
    throw std::invalid_argument(
        "Unable to create directory named: " + path.toStdString() +
        ".\n Does a folder with that name already exist?");
  }
}

void PluginCreator::copyTemplates(QString pluginName) {
  out << "Copying templates into the new directories..." << endl;

  QString proTemplate = templatesPath() + "template.pro";
  QString proDestination = srcPath(pluginName) + pluginName.toLower() + ".pro";
  copyFile(proTemplate, proDestination);
  out << "Copied project template to " << proDestination << endl;

  QString jsonTemplate = templatesPath() + "template.json";
  QString jsonDest = srcPath(pluginName) + pluginName.toLower() + ".json";
  copyFile(jsonTemplate, jsonDest);
  out << "Copied json template to " << jsonDest << endl;

  QString globalsTemplate = templatesPath() + "template_global.h";
  QString globalsDest = srcPath(pluginName) + pluginName.toLower() + "_global.h";
  copyFile(globalsTemplate, globalsDest);
  out << "Copied globals header template to " << globalsDest << endl;

  QString headerTemplate = templatesPath() + "headertemplate.h";
  QString headerDest = srcPath(pluginName) + pluginName.toLower() + ".h";
  copyFile(headerTemplate, headerDest);
  out << "Copied header template to " << headerDest << endl;

  QString sourceTemplate = templatesPath() + "sourcetemplate.cpp";
  QString sourceDest = srcPath(pluginName) + pluginName.toLower() + ".cpp";
  copyFile(sourceTemplate, sourceDest);
  out << "Copied source template to " << sourceDest << endl;

  QString widgetHeaderTemplate = templatesPath() + "widgettemplate.h";
  QString widgetHeaderDest = templatesPath() + pluginName.toLower() + "widget.h";
  copyFile(widgetHeaderTemplate, widgetHeaderDest);
  out << "Copied widget header template to " << widgetHeaderDest << endl;

  QString widgetSourceTemplate = templatesPath() + "widgettemplate.cpp";
  QString widgetSourceDest = templatesPath() + pluginName.toLower() + "widget.cpp";
  copyFile(widgetSourceTemplate, widgetSourceDest);
  out << "Copied widget source template to " << widgetSourceDest << endl;

  QString widgetFormTemplate = templatesPath() + "widgettemplate.ui";
  QString widgetFormDest = templatesPath() + pluginName.toLower() + "widget.ui";
  copyFile(widgetFormTemplate, widgetFormDest);
  out << "Copied widget form template to " << widgetFormDest << endl;
}

void PluginCreator::copyFile(QString from, QString to) {
  bool success = QFile::copy(from, to);
  if (!success) {
    throw std::invalid_argument(
        "Unable to copy file " + from.toStdString() + " to " +
        to.toStdString() + ".\n Does a file with the same name already exist?");
  }
}
