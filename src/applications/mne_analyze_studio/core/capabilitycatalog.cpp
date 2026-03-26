//=============================================================================================================
/**
 * @file     capabilitycatalog.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements helpers for building a normalized Studio capability catalog.
 */

#include "capabilitycatalog.h"

#include "capabilityutils.h"

#include <QHash>
#include <QSet>

using namespace MNEANALYZESTUDIO;

namespace
{

QStringList trimmedJsonStringList(const QJsonArray& values)
{
    QStringList strings;
    for(const QJsonValue& value : values) {
        const QString stringValue = value.toString().trimmed();
        if(!stringValue.isEmpty() && !strings.contains(stringValue)) {
            strings.append(stringValue);
        }
    }

    return strings;
}

QJsonArray stringListToJsonArray(const QStringList& values)
{
    return QJsonArray::fromStringList(values);
}

} // namespace

QJsonArray MNEANALYZESTUDIO::buildCapabilityCatalog(const QVector<CapabilityCatalogSource>& sources,
                                                    QStringList* warnings)
{
    QVector<QJsonObject> normalizedTools;
    QHash<QString, int> globalKeyCounts;
    QHash<QString, int> identityKeyCounts;

    for(const CapabilityCatalogSource& source : sources) {
        const QString sourceId = source.sourceId.trimmed();
        const QString sourceDisplayName = source.sourceDisplayName.trimmed();

        for(const QJsonValue& value : source.tools) {
            QJsonObject tool = annotateCapabilityMetadata(value.toObject());
            if(tool.isEmpty()) {
                continue;
            }

            if(!sourceId.isEmpty()) {
                tool.insert("capability_source_id", sourceId);
            }
            if(!sourceDisplayName.isEmpty()) {
                tool.insert("capability_source_display_name", sourceDisplayName);
            }

            const QString toolName = tool.value("name").toString().trimmed();
            const QString capabilityId = tool.value("capability_id").toString().trimmed();
            const QStringList aliases = trimmedJsonStringList(tool.value("capability_aliases").toArray());

            QStringList globalKeys;
            if(!toolName.isEmpty()) {
                globalKeys.append(toolName);
                identityKeyCounts.insert(toolName, identityKeyCounts.value(toolName, 0) + 1);
            }
            if(!capabilityId.isEmpty() && !globalKeys.contains(capabilityId)) {
                globalKeys.append(capabilityId);
                identityKeyCounts.insert(capabilityId, identityKeyCounts.value(capabilityId, 0) + 1);
            }
            for(const QString& alias : aliases) {
                if(!globalKeys.contains(alias)) {
                    globalKeys.append(alias);
                }
            }

            tool.insert("capability_catalog_keys", stringListToJsonArray(globalKeys));
            normalizedTools.append(tool);

            for(const QString& key : globalKeys) {
                globalKeyCounts.insert(key, globalKeyCounts.value(key, 0) + 1);
            }
        }
    }

    QSet<QString> emittedWarnings;
    QJsonArray catalog;
    for(QJsonObject tool : normalizedTools) {
        const QString toolName = tool.value("name").toString().trimmed();
        const QString capabilityId = tool.value("capability_id").toString().trimmed();
        const QStringList aliases = trimmedJsonStringList(tool.value("capability_aliases").toArray());

        QStringList lookupAliases;
        QStringList sharedAliases;
        for(const QString& alias : aliases) {
            if(globalKeyCounts.value(alias, 0) > 1) {
                if(!sharedAliases.contains(alias)) {
                    sharedAliases.append(alias);
                }
            } else if(!lookupAliases.contains(alias)) {
                lookupAliases.append(alias);
            }
        }

        QStringList lookupKeys;
        if(!toolName.isEmpty()) {
            lookupKeys.append(toolName);
        }
        if(!capabilityId.isEmpty() && !lookupKeys.contains(capabilityId)) {
            lookupKeys.append(capabilityId);
        }
        for(const QString& alias : lookupAliases) {
            if(!lookupKeys.contains(alias)) {
                lookupKeys.append(alias);
            }
        }

        QStringList conflicts;
        if(!toolName.isEmpty() && identityKeyCounts.value(toolName, 0) > 1) {
            conflicts.append(QStringLiteral("name:%1").arg(toolName));
        }
        if(!capabilityId.isEmpty() && identityKeyCounts.value(capabilityId, 0) > 1) {
            conflicts.append(QStringLiteral("capability_id:%1").arg(capabilityId));
        }

        tool.insert("capability_lookup_aliases", stringListToJsonArray(lookupAliases));
        tool.insert("capability_shared_aliases", stringListToJsonArray(sharedAliases));
        tool.insert("capability_lookup_keys", stringListToJsonArray(lookupKeys));
        tool.insert("capability_catalog_conflicts", stringListToJsonArray(conflicts));

        for(const QString& conflict : conflicts) {
            if(!warnings || emittedWarnings.contains(conflict)) {
                continue;
            }

            emittedWarnings.insert(conflict);
            warnings->append(QStringLiteral("Capability catalog conflict for `%1`.").arg(conflict));
        }

        catalog.append(tool);
    }

    return catalog;
}

QJsonObject MNEANALYZESTUDIO::capabilityFromCatalog(const QJsonArray& catalog, const QString& key)
{
    const QString trimmedKey = key.trimmed();
    if(trimmedKey.isEmpty()) {
        return QJsonObject();
    }

    for(const QJsonValue& value : catalog) {
        const QJsonObject tool = value.toObject();
        if(tool.value("name").toString().trimmed() == trimmedKey
           || tool.value("capability_id").toString().trimmed() == trimmedKey) {
            return tool;
        }
    }

    for(const QJsonValue& value : catalog) {
        const QJsonObject tool = value.toObject();
        const QStringList lookupKeys = trimmedJsonStringList(tool.value("capability_lookup_keys").toArray());
        if(lookupKeys.contains(trimmedKey)) {
            return tool;
        }
    }

    return QJsonObject();
}

QJsonArray MNEANALYZESTUDIO::capabilitiesFromCatalogBySource(const QJsonArray& catalog, const QString& sourceId)
{
    const QString trimmedSourceId = sourceId.trimmed();
    QJsonArray filtered;

    for(const QJsonValue& value : catalog) {
        const QJsonObject tool = value.toObject();
        if(tool.value("capability_source_id").toString().trimmed() == trimmedSourceId) {
            filtered.append(tool);
        }
    }

    return filtered;
}
