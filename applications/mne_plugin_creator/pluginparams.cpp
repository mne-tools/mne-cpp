#include "pluginparams.h"


  PluginParams::PluginParams(QString name, QString author, QString nameSpace) {
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
      m_globalHeaderDefine = baseDefine() + "_GLOBAL_H";
      m_libraryDefine = baseDefine() + "_LIBRARY";
      m_exportDefine = baseDefine() + "SHARED_EXPORT";
  }

 QString PluginParams::baseFileName() {
    return m_name.toLower();
 }

 QString PluginParams::baseDefine() {
     return m_name.toUpper();
 }
