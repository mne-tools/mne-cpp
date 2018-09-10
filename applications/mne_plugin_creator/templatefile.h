#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <stdexcept>

#include <QDate>
#include <QFile>

#include "pluginparams.h"

class TemplateFile {
public:
  TemplateFile(QString filepath, QString destinationPath);
  void fill(PluginParams &params);

private:
  QFile m_outfile;
  QFile m_template;
};

#endif // TEMPLATE_H
