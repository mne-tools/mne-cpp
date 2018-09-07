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

  PluginParams(QString name, QString author, QString nameSpace) {
      m_name = name;
      m_author = author;
      m_namespace = nameSpace;
      m_proFileName = baseFileName() + ".pro";
      m_jsonFileName = baseFileName() + ".json";
      m_globalsFileName = baseFileName() + "_global.h";
      m_headerFileName = baseFileName() + ".h";
      m_sourceFileName = baseFileName() + ".cpp";
      m_widgetHeaderFileName = baseFileName() + "widget.h";
      m_widgetSourceFileName = baseFileName() + "widget.cpp";
      m_widgetFormFileName = baseFileName() + "widget.ui";
      m_aboutWidgetHeaderFileName = baseFileName()  + "aboutwidget.h";
      m_aboutWidgetSourceFileName = baseFileName() + "aboutwidget.cpp";
      m_aboutWidgetFormFileName = baseFileName() + "aboutwidget.ui";
      m_setupWidgetHeaderFileName = baseFileName() + "setupwidget.h";
      m_setupWidgetSourceFileName = baseFileName() + "setupwidget.cpp";
      m_setupWidgetFormFileName = baseFileName() + "setupwidget.ui";
  }

  QString baseFileName() {
    return m_name.toLower();
  }
};

#endif // PLUGINPARAMS_H
