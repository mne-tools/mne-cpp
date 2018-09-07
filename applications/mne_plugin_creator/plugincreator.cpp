#include "plugincreator.h"

PluginCreator::PluginCreator(QObject *parent) : QObject(parent), out(stdout) {}

void PluginCreator::createPlugin(PluginParams &params) {
  out << "Creating plugin: " << params.m_name << "..." << endl;
  createFolderStructure(params.m_name);
  copyTemplates(params);
  fillTemplates(params);
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

QString PluginCreator::iconsPath(QString pluginName){
  return imagesPath(pluginName) + "icons/";
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

  createDirectory(iconsPath(pluginName));
  out << "Created icons folder at " << iconsPath(pluginName) << endl;
}

void PluginCreator::createDirectory(QString path) {
  bool success = QDir::current().mkdir(path);
  if (!success) {
    throw std::invalid_argument(
        "Unable to create directory named: " + path.toStdString() +
        ".\n Does a folder with that name already exist?");
  }
}

void PluginCreator::copyTemplates(PluginParams &params) {
  out << "Copying templates into the new directories..." << endl;

  QString proTemplate = templatesPath() + "template.pro";
  QString proDestination = srcPath(params.m_name) + params.m_proFileName;
  copyFile(proTemplate, proDestination);
  out << "Copied project template to " << proDestination << endl;

  QString jsonTemplate = templatesPath() + "template.json";
  QString jsonDest = srcPath(params.m_name) + params.m_jsonFileName;
  copyFile(jsonTemplate, jsonDest);
  out << "Copied json template to " << jsonDest << endl;

  QString optionsImage = templatesPath() + "options.png";
  QString optionsDest = iconsPath(params.m_name) + "options.png";
  copyFile(optionsImage, optionsDest);
  out << "Copied options icon to " << optionsDest << endl;

  QString globalsTemplate = templatesPath() + "template_global.h";
  QString globalsDest = srcPath(params.m_name) + params.m_globalsFileName;
  copyFile(globalsTemplate, globalsDest);
  out << "Copied globals header template to " << globalsDest << endl;

  QString headerTemplate = templatesPath() + "headertemplate.h";
  QString headerDest = srcPath(params.m_name) + params.m_headerFileName;
  copyFile(headerTemplate, headerDest);
  out << "Copied header template to " << headerDest << endl;

  QString sourceTemplate = templatesPath() + "sourcetemplate.cpp";
  QString sourceDest = srcPath(params.m_name) + params.m_sourceFileName;
  copyFile(sourceTemplate, sourceDest);
  out << "Copied source template to " << sourceDest << endl;

  QString widgetHeaderTemplate = templatesPath() + "widgettemplate.h";
  QString widgetHeaderDest = formsPath(params.m_name) + params.m_widgetHeaderFileName;
  copyFile(widgetHeaderTemplate, widgetHeaderDest);
  out << "Copied widget header template to " << widgetHeaderDest << endl;

  QString widgetSourceTemplate = templatesPath() + "widgettemplate.cpp";
  QString widgetSourceDest = formsPath(params.m_name) + params.m_widgetSourceFileName;
  copyFile(widgetSourceTemplate, widgetSourceDest);
  out << "Copied widget source template to " << widgetSourceDest << endl;

  QString widgetFormTemplate = templatesPath() + "widgettemplate.ui";
  QString widgetFormDest = formsPath(params.m_name) + params.m_widgetFormFileName;
  copyFile(widgetFormTemplate, widgetFormDest);
  out << "Copied widget form template to " << widgetFormDest << endl;

  QString aboutWidgetHeaderTemplate = templatesPath() + "aboutwidgettemplate.h";
  QString aboutWidgetHeaderDest = formsPath(params.m_name) + params.m_aboutWidgetHeaderFileName;
  copyFile(aboutWidgetHeaderTemplate, aboutWidgetHeaderDest);
  out << "Copied about widget header template to " << aboutWidgetHeaderDest << endl;

  QString aboutWidgetSourceTemplate = templatesPath() + "aboutwidgettemplate.cpp";
  QString aboutWidgetSourceDest = formsPath(params.m_name) + params.m_aboutWidgetSourceFileName;
  copyFile(aboutWidgetSourceTemplate, aboutWidgetSourceDest);
  out << "Copied about widget source template to " << aboutWidgetSourceDest << endl;

  QString aboutWidgetFormTemplate = templatesPath() + "aboutwidgettemplate.ui";
  QString aboutWidgetFormDest = formsPath(params.m_name) + params.m_aboutWidgetFormFileName;
  copyFile(aboutWidgetFormTemplate, aboutWidgetFormDest);
  out << "Copied about widget form template to " << aboutWidgetFormDest << endl;

  QString setupWidgetHeaderTemplate = templatesPath() + "setupwidgettemplate.h";
  QString setupWidgetHeaderDest = formsPath(params.m_name) + params.m_setupWidgetHeaderFileName;
  copyFile(setupWidgetHeaderTemplate, setupWidgetHeaderDest);
  out << "Copied setup widget header template to " << setupWidgetHeaderDest << endl;

  QString setupWidgetSourceTemplate = templatesPath() + "setupwidgettemplate.cpp";
  QString setupWidgetSourceDest = formsPath(params.m_name) + params.m_setupWidgetSourceFileName;
  copyFile(setupWidgetSourceTemplate, setupWidgetSourceDest);
  out << "Copied setup widget source template to " << setupWidgetSourceDest << endl;

  QString setupWidgetFormTemplate = templatesPath() + "setupwidgettemplate.ui";
  QString setupWidgetFormDest = formsPath(params.m_name) + params.m_setupWidgetFormFileName;
  copyFile(setupWidgetFormTemplate, setupWidgetFormDest);
  out << "Copied setup widget form template to " << setupWidgetFormDest << endl;
}

void PluginCreator::copyFile(QString from, QString to) {
  bool success = QFile::copy(from, to);
  if (!success) {
    throw std::invalid_argument(
        "Unable to copy file " + from.toStdString() + " to " +
        to.toStdString() + ".\n Does a file with the same name already exist?");
  }
}

void PluginCreator::fillTemplates(PluginParams &params){
   QStringList nameFilters;

   QDir rootDir(srcPath(params.m_name));
   QFileInfoList srcFiles = rootDir.entryInfoList(nameFilters, QDir::Files);

   QDir formsDir(formsPath(params.m_name));
   QFileInfoList formFiles = formsDir.entryInfoList(nameFilters, QDir::Files);

   for (QFileInfo info : formFiles + srcFiles) {
       QFile file(info.absoluteFilePath());
       fillSingleTemplate(file, params);
       out << "Executed template for " << file.fileName() << endl;
   }
}

void PluginCreator::fillSingleTemplate(QFile &file, PluginParams &params) {
    if (!file.exists()) {
        throw std::invalid_argument("File named: " + file.fileName().toStdString() +
                                    " could not be found!");
    }
    const bool success = file.open(QIODevice::ReadWrite | QIODevice::Text);
    if (!success) {
        QString filename = file.fileName();
        QString problem = file.errorString();
        throw std::runtime_error("Unable to open file: " + filename.toStdString() +
                                 "\nError: " + problem.toStdString());
    }

    QDate date = QDate::currentDate();
    QByteArray text = file.readAll();

    text.replace("{{author}}", params.m_author.toUtf8());
    text.replace("{{author_email}}", params.m_email.toUtf8());
    text.replace("{{month}}", date.toString("MMMM").toUtf8());
    text.replace("{{year}}", date.toString("yyyy").toUtf8());
    text.replace("{{name}}", params.m_name.toUtf8());
    text.replace("{{global_header_filename}}", params.m_globalsFileName.toUtf8());
    text.replace("{{header_define}}", params.m_globalHeaderDefine.toUtf8());
    text.replace("{{library_define}}", params.m_libraryDefine.toUtf8());
    text.replace("{{export_define}}", params.m_exportDefine.toUtf8());

    file.resize(0);
    file.write(text);
    file.close();
}
