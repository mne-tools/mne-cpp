#ifndef PLUGINPARAMS_H
#define PLUGINPARAMS_H

#include <QString>

struct PluginParams {
    PluginParams(const QString& name, const QString& superclass, const QString& nameSpace, const QString author, const QString& email);

    QString m_name;
    QString m_superclass;
    QString m_widgetName;
    QString m_aboutWidgetName;
    QString m_setupWidgetName;
    QString m_author;
    QString m_email;
    QString m_namespace;
    QString m_targetName;

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

    QString m_globalHeaderDefine;
    QString m_libraryDefine;
    QString m_exportDefine;
    QString m_headerDefine;
    QString m_widgetHeaderDefine;
    QString m_setupWidgetHeaderDefine;
    QString m_aboutWidgetHeaderDefine;
};

#endif // PLUGINPARAMS_H
