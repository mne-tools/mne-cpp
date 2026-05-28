//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_op_registry.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of the @ref MnaOpRegistry singleton, including the upward search for @c resources/mna/ and the drop-in registry merge logic.
 *
 * The registry stores schemas and op functions in two
 * @c QMap members keyed by op type. @ref registerOp and
 * @ref registerOpFunc both replace any existing entry so plug-ins
 * loaded later can shadow built-ins by design. @ref loadRegistryFiles
 * walks upward from @c QCoreApplication::applicationDirPath looking
 * for a @c resources/mna/ directory, then delegates to
 * @ref MnaRegistryLoader::loadDirectory, which in turn reads the
 * master @c mna-registry.json plus every drop-in under
 * @c mna-registry.d/ in alphabetical order. @ref missingOps walks
 * a supplied tool list and returns those entries with no
 * registered schema, which is the simplest way to detect a stale
 * or partial install before a graph is executed.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_op_registry.h"
#include "mna_registry_loader.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDir>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnaOpRegistry::MnaOpRegistry()
{
    loadRegistryFiles();
}

//=============================================================================================================

MnaOpRegistry& MnaOpRegistry::instance()
{
    static MnaOpRegistry registry;
    return registry;
}

//=============================================================================================================

void MnaOpRegistry::registerOp(const MnaOpSchema& schema)
{
    m_schemas.insert(schema.opType, schema);
}

//=============================================================================================================

bool MnaOpRegistry::hasOp(const QString& opType) const
{
    return m_schemas.contains(opType);
}

//=============================================================================================================

MnaOpSchema MnaOpRegistry::schema(const QString& opType) const
{
    return m_schemas.value(opType);
}

//=============================================================================================================

QStringList MnaOpRegistry::registeredOps() const
{
    return m_schemas.keys();
}

//=============================================================================================================

void MnaOpRegistry::registerOpFunc(const QString& opType, OpFunc func)
{
    m_funcs.insert(opType, func);
}

//=============================================================================================================

MnaOpRegistry::OpFunc MnaOpRegistry::opFunc(const QString& opType) const
{
    return m_funcs.value(opType);
}

//=============================================================================================================

int MnaOpRegistry::loadRegistryFiles()
{
    // Search for resources/mna/ directory, walking up from the application dir
    const QString relPath = QStringLiteral("resources/mna");

    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        QString candidate = dir.absoluteFilePath(relPath);
        if (QDir(candidate).exists()) {
            return MnaRegistryLoader::loadDirectory(candidate, *this);
        }
        dir.cdUp();
    }

    // Try from current working directory
    if (QDir(relPath).exists())
        return MnaRegistryLoader::loadDirectory(QDir::currentPath() + QStringLiteral("/") + relPath, *this);

    qWarning() << "MnaOpRegistry: Could not find resources/mna/ registry directory";
    return 0;
}

//=============================================================================================================

QStringList MnaOpRegistry::missingOps(const QStringList& pipelineTools) const
{
    QStringList missing;
    for (const QString& tool : pipelineTools) {
        if (!m_schemas.contains(tool))
            missing.append(tool);
    }
    return missing;
}
