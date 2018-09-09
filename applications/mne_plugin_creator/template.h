#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <stdexcept>

#include <QDate>
#include <QFile>

#include "pluginparams.h"

class Template {
public:
  Template(QString filepath);
  void fill(PluginParams &params);

private:
  QFile m_file;
};

#endif // TEMPLATE_H
