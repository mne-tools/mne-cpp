//=============================================================================================================
/**
 * @file     mna_op_registry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MnaOpRegistry class implementation.
 *
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
