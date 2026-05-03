//=============================================================================================================
/**
 * @file     readmnaskill.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    ReadMnaSkill implementation.
 */

#include "readmnaskill.h"

#include <workflowgraph.h>

#include <mna/mna_io.h>
#include <mna/mna_project.h>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

using namespace MNEANALYZESTUDIO;
using namespace MNALIB;

namespace {

QJsonObject objectSchema(const QJsonObject& properties, const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

QString resolveUri(const QString& uri)
{
    if (uri.startsWith(QLatin1String("file://"))) {
        return uri.mid(7);
    }
    return uri;
}

} // namespace

ReadMnaSkill::ReadMnaSkill(QObject* parent)
    : ISkillOperator(parent)
{
}

QJsonObject ReadMnaSkill::getOperatorDefinition() const
{
    return QJsonObject{
        {"skill_id", "mne.skills.read_mna"},
        {"tool_name", "read_mna_project"},
        {"display_name", "Read MNA Project"},
        {"description", "Load an .mna or .mnx project file and expose its summary as workflow output."},
        {"extension_id", "mna-skills"},
        {"extension_display_name", "MNA Project Skills"},
        {"inputs_schema", objectSchema(QJsonObject{
            {"path", QJsonObject{
                {"type", "string"},
                {"title", "Project path"},
                {"description", "Path to an .mna (JSON) or .mnx (CBOR) project file."}
            }}
        }, QJsonArray{"path"})},
        {"parameters_schema", objectSchema(QJsonObject{})},
        {"outputs_schema", objectSchema(QJsonObject{
            {"project_summary", QJsonObject{
                {"type", "object"},
                {"title", "Project summary"},
                {"description", "Name, schema version, subject count and pipeline length."}
            }},
            {"project_path", QJsonObject{
                {"type", "string"},
                {"title", "Resolved project path"}
            }}
        }, QJsonArray{"project_summary", "project_path"})}
    };
}

QJsonObject ReadMnaSkill::executeSkill(const WorkflowNode& nodeState)
{
    const QJsonObject pathInput = nodeState.resolvedInputs.value("path").toObject();
    QString uri = pathInput.value("uri").toString();
    if (uri.isEmpty()) {
        // Fall back to a plain string input.
        uri = nodeState.resolvedInputs.value("path").toString();
    }
    if (uri.isEmpty()) {
        return QJsonObject{
            {"status", "error"},
            {"message", QStringLiteral("Read MNA node `%1`: missing `path` input.").arg(nodeState.uid)}
        };
    }

    const QString path = resolveUri(uri);
    if (!QFileInfo::exists(path)) {
        return QJsonObject{
            {"status", "error"},
            {"message", QStringLiteral("Read MNA node `%1`: file `%2` not found.").arg(nodeState.uid, path)}
        };
    }

    const MnaProject project = MnaIO::read(path);

    QJsonObject summary{
        {"name",            project.name},
        {"description",     project.description},
        {"mna_version",     project.mnaVersion},
        {"subject_count",   project.subjects.size()},
        {"pipeline_length", project.pipeline.size()}
    };

    return QJsonObject{
        {"status", "completed"},
        {"message", QStringLiteral("Loaded MNA project `%1` (%2 subjects, %3 pipeline nodes).")
                        .arg(project.name).arg(project.subjects.size()).arg(project.pipeline.size())},
        {"outputs", QJsonObject{
            {"project_summary", summary},
            {"project_path",    path}
        }}
    };
}
