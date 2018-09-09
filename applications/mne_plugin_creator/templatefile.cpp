#include "templatefile.h"

TemplateFile::TemplateFile(QString filepath)
  : m_file(filepath)
{}

void
TemplateFile::fill(PluginParams& params)
{
  if (!m_file.exists()) {
    throw std::invalid_argument("File named: " + m_file.fileName().toStdString() + " could not be found!");
  }

  const bool success = m_file.open(QIODevice::ReadWrite | QIODevice::Text);
  if (!success) {
    QString filename = m_file.fileName();
    QString problem = m_file.errorString();
    throw std::runtime_error("Unable to open file: " + filename.toStdString() + "\nError: " + problem.toStdString());
  }

  QDate date = QDate::currentDate();
  QByteArray text = m_file.readAll();

  text.replace("{{author}}", params.m_author.toUtf8());
  text.replace("{{author_email}}", params.m_email.toUtf8());
  text.replace("{{month}}", date.toString("MMMM").toUtf8());
  text.replace("{{year}}", date.toString("yyyy").toUtf8());
  text.replace("{{namespace}}", params.m_namespace.toUtf8());
  text.replace("{{target_name}}", params.m_targetName.toUtf8());

  text.replace("{{name}}", params.m_name.toUtf8());
  text.replace("{{widget_name}}", params.m_widgetName.toUtf8());
  text.replace("{{setup_widget_name}}", params.m_setupWidgetName.toUtf8());
  text.replace("{{about_widget_name}}", params.m_aboutWidgetName.toUtf8());

  text.replace("{{header_define}}", params.m_globalHeaderDefine.toUtf8());
  text.replace("{{library_define}}", params.m_libraryDefine.toUtf8());
  text.replace("{{export_define}}", params.m_exportDefine.toUtf8());
  text.replace("{{header_define}}", params.m_headerDefine.toUtf8());
  text.replace("{{widget_header_define}}", params.m_widgetHeaderDefine.toUtf8());
  text.replace("{{setup_widget_header_define}}", params.m_setupWidgetHeaderDefine.toUtf8());
  text.replace("{{about_widget_header_define}}", params.m_aboutWidgetHeaderDefine.toUtf8());

  text.replace("{{pro_filename}}", params.m_proFileName.toUtf8());
  text.replace("{{json_filename}}", params.m_jsonFileName.toUtf8());

  text.replace("{{global_header_filename}}", params.m_globalsFileName.toUtf8());
  text.replace("{{header_filename}}", params.m_headerFileName.toUtf8());
  text.replace("{{widget_header_filename}}", params.m_widgetHeaderFileName.toUtf8());
  text.replace("{{setup_widget_header_filename}}", params.m_setupWidgetHeaderFileName.toUtf8());
  text.replace("{{about_widget_header_filename}}", params.m_aboutWidgetHeaderFileName.toUtf8());

  text.replace("{{source_filename}}", params.m_sourceFileName.toUtf8());
  text.replace("{{widget_source_filename}}", params.m_widgetSourceFileName.toUtf8());
  text.replace("{{setup_widget_source_filename}}", params.m_setupWidgetSourceFileName.toUtf8());
  text.replace("{{about_widget_source_filename}}", params.m_aboutWidgetSourceFileName.toUtf8());

  text.replace("{{widget_form_filename}}", params.m_widgetFormFileName.toUtf8());
  text.replace("{{setup_widget_form_filename}}", params.m_setupWidgetFormFileName.toUtf8());
  text.replace("{{about_widget_form_filename}}", params.m_aboutWidgetFormFileName.toUtf8());

  m_file.resize(0);
  m_file.write(text);
  m_file.close();
}
