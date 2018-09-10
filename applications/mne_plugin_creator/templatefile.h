#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <stdexcept>

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "pluginparams.h"

class TemplateFile {
public:
  TemplateFile(const QString &filepath, const QString &destinationPath);
  void fill(const PluginParams &params);

private:
  QFile m_outfile;
  QFile m_template;
};

#endif // TEMPLATE_H
