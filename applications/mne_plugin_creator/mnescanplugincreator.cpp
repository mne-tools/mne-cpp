#include "mnescanplugincreator.h"

MNEScanPluginCreator::MNEScanPluginCreator():
    m_templatesPath("../../../mne-cpp/applications/mne_plugin_creator/templates/"),
    m_pluginsPath("../../../mne-cpp/applications/mne_scan/plugins/"),
    m_profilePath(m_pluginsPath + "plugins.pro")
{
}

QList<QPair<QString, QString>> MNEScanPluginCreator::templateInputOutputPairs(const PluginParams &params) const {
    QList<QPair<QString, QString>> pairs;
    pairs.append(QPair<QString, QString>("1", "2"));

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
