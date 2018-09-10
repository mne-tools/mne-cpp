#include "pluginparams.h"


  PluginParams::PluginParams(QString name, QString nameSpace, QString author, QString email) {

      // Basic parameters
      m_email = email;
      m_author = author;
      m_namespace = nameSpace;
      m_targetName = baseFileName();

      // Class names
      m_name = name;
      m_widgetName= name + "Widget";
      m_aboutWidgetName = name + "AboutWidget";
      m_setupWidgetName = name + "SetupWidget";

      // File names
      m_proFileName = baseFileName() + ".pro";
      m_jsonFileName = baseFileName() + ".json";
      m_globalsFileName = baseFileName() + "_global.h";

      // Header files
      m_headerFileName = baseFileName() + ".h";
      m_widgetHeaderFileName = baseFileName() + "widget.h";
      m_aboutWidgetHeaderFileName = baseFileName()  + "aboutwidget.h";
      m_setupWidgetHeaderFileName = baseFileName() + "setupwidget.h";

      // Source files
      m_sourceFileName = baseFileName() + ".cpp";
      m_widgetSourceFileName = baseFileName() + "widget.cpp";
      m_aboutWidgetSourceFileName = baseFileName() + "aboutwidget.cpp";
      m_setupWidgetSourceFileName = baseFileName() + "setupwidget.cpp";

      // Form files
      m_widgetFormFileName = baseFileName() + "widget.ui";
      m_aboutWidgetFormFileName = baseFileName() + "aboutwidget.ui";
      m_setupWidgetFormFileName = baseFileName() + "setupwidget.ui";

      // #define
      m_globalHeaderDefine = baseDefine() + "_GLOBAL_H";
      m_libraryDefine = baseDefine() + "_LIBRARY";
      m_exportDefine = baseDefine() + "SHARED_EXPORT";
      m_headerDefine = baseDefine() + "_H";
      m_widgetHeaderDefine = baseDefine() + "WIDGET_H";
      m_setupWidgetHeaderDefine = baseDefine() + "SETUPWIDGET_H";
      m_aboutWidgetHeaderDefine = baseDefine() + "ABOUTWIDGET_H";
  }

 QString PluginParams::baseFileName() {
    return m_name.toLower();
 }

 QString PluginParams::baseDefine() {
     return m_name.toUpper();
 }
