#include "mnescanplugincreator.h"

MNEScanPluginCreator::MNEScanPluginCreator():
    m_templatesPath("../../../mne-cpp/applications/mne_plugin_creator/templates/"),
    m_pluginsPath("../../../mne-cpp/applications/mne_scan/plugins/"),
    m_profilePath(m_pluginsPath + "plugins.pro")
{
}

QList<QPair<QString, QString>> MNEScanPluginCreator::templateInputOutputPairs(const PluginParams &params) const {
    const QString name = params.m_name;
    const QString root = srcPath(name);
    const QString icons = iconsPath(name);
    const QString forms = formsPath(name);
    QList<QPair<QString, QString>> pairs;

    // Config files, etc.
    pairs.append(QPair<QString, QString>(m_templatesPath + "template.pro", root + params.m_proFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "template.json", root + params.m_jsonFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "options.json", icons + "options.png"));

    // Header files
    pairs.append(QPair<QString, QString>(m_templatesPath + "template_global.h", root + params.m_globalsFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "headertemplate.h", root + params.m_headerFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "widgettemplate.h", forms + params.m_widgetHeaderFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "setupwidgettemplate.h", forms + params.m_setupWidgetHeaderFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "aboutwidgettemplate.h", forms + params.m_aboutWidgetHeaderFileName));

    // Source files
    pairs.append(QPair<QString, QString>(m_templatesPath + "sourcetemplate.cpp", root + params.m_sourceFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "widgettemplate.cpp", forms + params.m_widgetSourceFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "setupwidgettemplate.cpp", forms + params.m_setupWidgetSourceFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "aboutwidgettemplate.cpp", forms + params.m_aboutWidgetSourceFileName));

    // Form files
    pairs.append(QPair<QString, QString>(m_templatesPath + "widgettemplate.ui", forms + params.m_widgetFormFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "setupwidgettemplate.ui", forms + params.m_setupWidgetFormFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "aboutwidgettemplate.ui", forms + params.m_aboutWidgetFormFileName));

    return pairs;
}

void MNEScanPluginCreator::updateProjectFile(const PluginParams &params) const
{
    QFile proFile(m_profilePath);
    if (!proFile.exists()) {
      throw std::invalid_argument(m_profilePath.toStdString() + "could not be found!");
    }

    const bool success = proFile.open(QIODevice::ReadWrite | QIODevice::Text);
    if (!success) {
     QString filename = proFile.fileName();
      QString problem = proFile.errorString();
      throw std::runtime_error("Unable to open profile: " + filename.toStdString() + "\nError: " + problem.toStdString());
    }

    QByteArray text = proFile.readAll();
    QRegularExpression regex;
    regex.setPattern("\\s*Algorithms\\s*(SUBDIRS\\s*\\+=\\s*\\\\)");
    regex.setPatternOptions(QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator matches = regex.globalMatch(text);

    // We only want to operate on the second match. The first match is the minimal build. See plugins.pro
    matches.next();
    text.insert(matches.next().capturedEnd(), "\n\t" + folderName(params.m_name) + " \\");

    proFile.resize(0);
    proFile.write(text);
    proFile.close();

    qDebug() << "Updated the .pro file to include your new plugin!" << endl;
}

QString MNEScanPluginCreator::folderName(const QString &pluginName) const {
    return pluginName.toLower();
}

QString MNEScanPluginCreator::srcPath(const QString &pluginName) const {
    return m_pluginsPath + folderName(pluginName) + "/";
}

QString MNEScanPluginCreator::iconsPath(const QString &pluginName) const {
    return srcPath(pluginName) + "images/icons/";
}

QString MNEScanPluginCreator::formsPath(const QString &pluginName) const {
    return srcPath(pluginName) + "FormFiles/";
}
