#include "templatefile.h"

TemplateFile::TemplateFile(const QString& filepath, const QString& destinationPath)
    : m_outfile(destinationPath)
    , m_template(filepath)
{
}

void TemplateFile::fill(const PluginParams& params)
{
    // Make sure the template file exists and can be opened.
    if (!m_template.exists()) {
        throw std::invalid_argument("File named: " + m_template.fileName().toStdString() + " could not be found!");
    }

    const bool templateSuccess = m_template.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!templateSuccess) {
        QString filename = m_template.fileName();
        QString problem = m_template.errorString();
        throw std::invalid_argument("Unable to open file: " + filename.toStdString() + "\nError: " + problem.toStdString());
    }

    // If the output destination does not exist yet, create it.
    // Note that `QDir::mkpath` will return `true` if the folder already exists.
    const QString folder = QFileInfo(m_outfile).absolutePath();
    const bool mkdirSuccess = QDir::current().mkpath(folder);
    if (!mkdirSuccess) {
        throw new std::invalid_argument("Unable to create folder: " + folder.toStdString());
    }

    // Opening a non-existent file in `WriteOnly` mode will create a new file.
    const bool outputSuccess = m_outfile.open(QIODevice::WriteOnly | QIODevice::Text);
    if (!outputSuccess) {
        QString filename = m_outfile.fileName();
        QString problem = m_outfile.errorString();
        throw std::invalid_argument("Unable to open file: " + filename.toStdString() + "\nError: " + problem.toStdString());
    }

    // The template files all have variable names surrounded by {{ }} that we can search for and replace.
    QDate date = QDate::currentDate();
    QByteArray text = m_template.readAll();
    m_template.close();

    text.replace("{{author}}", params.m_author.toUtf8());
    text.replace("{{author_email}}", params.m_email.toUtf8());
    text.replace("{{month}}", date.toString("MMMM").toUtf8());
    text.replace("{{year}}", date.toString("yyyy").toUtf8());
    text.replace("{{namespace}}", params.m_namespace.toUtf8());
    text.replace("{{target_name}}", params.m_targetName.toUtf8());

    text.replace("{{name}}", params.m_name.toUtf8());
    text.replace("{{superclass}}", params.m_superclass.toUtf8());
    text.replace("{{widget_name}}", params.m_widgetName.toUtf8());
    text.replace("{{setup_widget_name}}", params.m_setupWidgetName.toUtf8());
    text.replace("{{about_widget_name}}", params.m_aboutWidgetName.toUtf8());

    text.replace("{{global_header_define}}", params.m_globalHeaderDefine.toUtf8());
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

    m_outfile.resize(0);
    m_outfile.write(text);
    m_outfile.close();
}
