#ifndef PLUGINPARAMS_H
#define PLUGINPARAMS_H

#include <QString>

struct PluginParams {
  QString m_name;
  QString m_author;
  QString m_namespace;

  QString m_proFileName;
  QString m_jsonFileName;
  QString m_globalsFileName;

  QString m_headerFileName;
  QString m_sourceFileName;

  QString m_widgetHeaderFileName;
  QString m_widgetSourceFileName;
  QString m_widgetFormFileName;

  QString m_aboutWidgetHeaderFileName;
  QString m_aboutWidgetSourceFileName;
  QString m_aboutWidgetFormFileName;

  QString m_setupWidgetHeaderFileName;
  QString m_setupWidgetSourceFileName;
  QString m_setupWidgetFormFileName;

  PluginParams(QString name, QString author, QString nameSpace);
  QString baseFileName();
};

#endif // PLUGINPARAMS_H
