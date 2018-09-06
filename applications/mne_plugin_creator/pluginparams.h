#ifndef PLUGINPARAMS_H
#define PLUGINPARAMS_H

#include <QString>

struct PluginParams {
  QString m_name;
  QString m_author;
  QString m_namespace;

  PluginParams(QString name, QString author, QString nameSpace)
      : m_name(name), m_author(author), m_namespace(nameSpace) {}
};

#endif // PLUGINPARAMS_H
