//=============================================================================================================
/**
 * @file     virtualchannelmodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
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
 * @brief    Definition of the VirtualChannelModel class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "virtualchannelmodel.h"

#include <QBrush>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace MNEBROWSE;

namespace
{

QString kindToString(VirtualChannelKind kind)
{
    switch(kind) {
        case VirtualChannelKind::AverageReference:
            return QStringLiteral("average_reference");
        case VirtualChannelKind::WeightedReference:
            return QStringLiteral("weighted_reference");
        case VirtualChannelKind::Bipolar:
        default:
            return QStringLiteral("bipolar");
    }
}

VirtualChannelKind stringToKind(const QString& text)
{
    const QString normalizedText = text.trimmed();
    if(normalizedText.compare(QStringLiteral("average_reference"), Qt::CaseInsensitive) == 0) {
        return VirtualChannelKind::AverageReference;
    }

    if(normalizedText.compare(QStringLiteral("weighted_reference"), Qt::CaseInsensitive) == 0) {
        return VirtualChannelKind::WeightedReference;
    }

    return VirtualChannelKind::Bipolar;
}

QString kindToDisplayString(VirtualChannelKind kind)
{
    switch(kind) {
        case VirtualChannelKind::AverageReference:
            return QStringLiteral("Average Reference");
        case VirtualChannelKind::WeightedReference:
            return QStringLiteral("Weighted Reference");
        case VirtualChannelKind::Bipolar:
        default:
            return QStringLiteral("Bipolar");
    }
}

QString joinedReferenceChannels(const QStringList& referenceChannels)
{
    return referenceChannels.join(QStringLiteral(", "));
}

QString formatWeight(double weight)
{
    return QString::number(weight, 'g', 6);
}

} // namespace

//=============================================================================================================

VirtualChannelModel::VirtualChannelModel(QObject *parent)
: QAbstractTableModel(parent)
{
}

//=============================================================================================================

VirtualChannelModel::~VirtualChannelModel() = default;

//=============================================================================================================

int VirtualChannelModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_virtualChannels.size();
}

//=============================================================================================================

int VirtualChannelModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 5;
}

//=============================================================================================================

QVariant VirtualChannelModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole) {
        return QVariant();
    }

    if(role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0:
                return QStringLiteral("Name");
            case 1:
                return QStringLiteral("Type");
            case 2:
                return QStringLiteral("Source");
            case 3:
                return QStringLiteral("Reference(s)");
            case 4:
                return QStringLiteral("Formula");
            default:
                break;
        }
    } else if(orientation == Qt::Vertical) {
        return QStringLiteral("Virtual %1").arg(section + 1);
    }

    return QVariant();
}

//=============================================================================================================

QVariant VirtualChannelModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_virtualChannels.size()) {
        return QVariant();
    }

    const VirtualChannelDefinition& definition = m_virtualChannels.at(index.row());

    if(role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
    }

    if(role == Qt::BackgroundRole) {
        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        brush.setColor(QColor(40, 120, 180, 28));
        return QVariant(brush);
    }

    if(role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    switch(index.column()) {
        case 0:
            return definition.name;
        case 1:
            return kindToDisplayString(definition.kind);
        case 2:
            return definition.primaryChannel;
        case 3:
            return referenceSummaryForDefinition(definition);
        case 4:
            return formulaForDefinition(definition);
        default:
            return QVariant();
    }
}

//=============================================================================================================

Qt::ItemFlags VirtualChannelModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()) {
        return Qt::NoItemFlags;
    }

    if(index.column() == 0) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

//=============================================================================================================

bool VirtualChannelModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role != Qt::EditRole || !index.isValid()
       || index.row() < 0 || index.row() >= m_virtualChannels.size()
       || index.column() != 0) {
        return false;
    }

    VirtualChannelDefinition& definition = m_virtualChannels[index.row()];
    definition = normalizeDefinition(value.toString(),
                                     definition.kind,
                                     definition.primaryChannel,
                                     definition.referenceChannels,
                                     definition.referenceWeights,
                                     definition.referenceSetName);

    emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1));
    notifyVirtualChannelsChanged();
    return true;
}

//=============================================================================================================

bool VirtualChannelModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    Q_UNUSED(parent)

    if(position < 0 || rows <= 0 || position + rows > m_virtualChannels.size()) {
        return false;
    }

    beginRemoveRows(QModelIndex(), position, position + rows - 1);
    for(int index = 0; index < rows; ++index) {
        m_virtualChannels.removeAt(position);
    }
    endRemoveRows();

    notifyVirtualChannelsChanged();
    return true;
}

//=============================================================================================================

int VirtualChannelModel::addVirtualChannel(const QString& name,
                                           VirtualChannelKind kind,
                                           const QString& primaryChannel,
                                           const QStringList& referenceChannels,
                                           const QVector<double>& referenceWeights,
                                           const QString& referenceSetName)
{
    const VirtualChannelDefinition definition =
        normalizeDefinition(name, kind, primaryChannel, referenceChannels, referenceWeights, referenceSetName);

    const int row = m_virtualChannels.size();
    beginInsertRows(QModelIndex(), row, row);
    m_virtualChannels.append(definition);
    endInsertRows();

    notifyVirtualChannelsChanged();
    return row;
}

//=============================================================================================================

int VirtualChannelModel::addReferenceSet(const QString& name, const QStringList& channels)
{
    const VirtualReferenceSetDefinition definition = normalizeReferenceSet(name, channels);
    if(definition.name.isEmpty() || definition.channels.isEmpty()) {
        return -1;
    }

    beginResetModel();

    int setIndex = -1;
    for(int index = 0; index < m_referenceSets.size(); ++index) {
        if(m_referenceSets.at(index).name.compare(definition.name, Qt::CaseSensitive) == 0) {
            m_referenceSets[index] = definition;
            setIndex = index;
            break;
        }
    }

    if(setIndex < 0) {
        setIndex = m_referenceSets.size();
        m_referenceSets.append(definition);
    }

    endResetModel();
    notifyVirtualChannelsChanged();
    return setIndex;
}

//=============================================================================================================

bool VirtualChannelModel::removeReferenceSet(const QString& name)
{
    const QString trimmedName = name.trimmed();
    if(trimmedName.isEmpty()) {
        return false;
    }

    int removeIndex = -1;
    for(int index = 0; index < m_referenceSets.size(); ++index) {
        if(m_referenceSets.at(index).name == trimmedName) {
            removeIndex = index;
            break;
        }
    }

    if(removeIndex < 0) {
        return false;
    }

    beginResetModel();
    m_referenceSets.removeAt(removeIndex);
    for(VirtualChannelDefinition& definition : m_virtualChannels) {
        if(definition.referenceSetName == trimmedName) {
            definition.referenceSetName.clear();
        }
    }
    endResetModel();

    notifyVirtualChannelsChanged();
    return true;
}

//=============================================================================================================

QVector<VirtualChannelDefinition> VirtualChannelModel::virtualChannels() const
{
    QVector<VirtualChannelDefinition> resolvedDefinitions = m_virtualChannels;
    for(VirtualChannelDefinition& definition : resolvedDefinitions) {
        definition.referenceChannels = resolvedReferenceChannels(definition);
        definition.referenceWeights = resolvedReferenceWeights(definition, definition.referenceChannels.size());
    }

    return resolvedDefinitions;
}

//=============================================================================================================

QVector<VirtualReferenceSetDefinition> VirtualChannelModel::referenceSets() const
{
    return m_referenceSets;
}

//=============================================================================================================

QStringList VirtualChannelModel::referenceSetNames() const
{
    QStringList names;
    names.reserve(m_referenceSets.size());
    for(const VirtualReferenceSetDefinition& referenceSet : m_referenceSets) {
        names.append(referenceSet.name);
    }

    return names;
}

//=============================================================================================================

VirtualReferenceSetDefinition VirtualChannelModel::referenceSet(const QString& name) const
{
    const QString trimmedName = name.trimmed();
    for(const VirtualReferenceSetDefinition& definition : m_referenceSets) {
        if(definition.name == trimmedName) {
            return definition;
        }
    }

    return {};
}

//=============================================================================================================

bool VirtualChannelModel::loadVirtualChannels(QFile& qFile)
{
    if(!qFile.open(QIODevice::ReadOnly)) {
        qWarning() << "VirtualChannelModel: could not open virtual-channel file" << qFile.fileName();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(qFile.readAll(), &parseError);
    qFile.close();

    if(parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "VirtualChannelModel: invalid virtual-channel json"
                   << qFile.fileName() << parseError.errorString();
        return false;
    }

    const QJsonObject rootObject = document.object();
    const QJsonArray setArray = rootObject.value(QStringLiteral("reference_sets")).toArray();
    const QJsonArray channelArray = rootObject.value(QStringLiteral("virtual_channels")).toArray();

    beginResetModel();
    m_referenceSets.clear();
    m_virtualChannels.clear();

    for(const QJsonValue& value : setArray) {
        const QJsonObject object = value.toObject();
        if(object.isEmpty()) {
            continue;
        }

        QStringList channels;
        const QJsonArray channelNameArray = object.value(QStringLiteral("channels")).toArray();
        for(const QJsonValue& channelValue : channelNameArray) {
            const QString channelName = channelValue.toString().trimmed();
            if(!channelName.isEmpty() && !channels.contains(channelName)) {
                channels.append(channelName);
            }
        }

        const VirtualReferenceSetDefinition definition =
            normalizeReferenceSet(object.value(QStringLiteral("name")).toString(), channels);
        if(!definition.name.isEmpty() && !definition.channels.isEmpty()) {
            m_referenceSets.append(definition);
        }
    }

    for(const QJsonValue& value : channelArray) {
        const QJsonObject object = value.toObject();
        if(object.isEmpty()) {
            continue;
        }

        QStringList referenceChannels;
        const QJsonValue referenceValue = object.value(QStringLiteral("reference_channels"));
        if(referenceValue.isArray()) {
            const QJsonArray referenceArray = referenceValue.toArray();
            for(const QJsonValue& referenceChannelValue : referenceArray) {
                const QString referenceChannel = referenceChannelValue.toString().trimmed();
                if(!referenceChannel.isEmpty() && !referenceChannels.contains(referenceChannel)) {
                    referenceChannels.append(referenceChannel);
                }
            }
        }

        QVector<double> referenceWeights;
        const QJsonValue weightsValue = object.value(QStringLiteral("reference_weights"));
        if(weightsValue.isArray()) {
            const QJsonArray weightsArray = weightsValue.toArray();
            referenceWeights.reserve(weightsArray.size());
            for(const QJsonValue& weightValue : weightsArray) {
                referenceWeights.append(weightValue.toDouble(1.0));
            }
        }

        VirtualChannelKind kind = stringToKind(object.value(QStringLiteral("type")).toString());
        QString primaryChannel = object.value(QStringLiteral("primary_channel")).toString();
        QString referenceSetName = object.value(QStringLiteral("reference_set_name")).toString().trimmed();

        // Backward compatibility for the older bipolar-only sidecar format.
        if(primaryChannel.isEmpty()) {
            primaryChannel = object.value(QStringLiteral("positive_channel")).toString();
        }

        if(referenceChannels.isEmpty()) {
            const QString negativeChannel = object.value(QStringLiteral("negative_channel")).toString();
            if(!negativeChannel.trimmed().isEmpty()) {
                referenceChannels.append(negativeChannel.trimmed());
            }
        }

        m_virtualChannels.append(normalizeDefinition(object.value(QStringLiteral("name")).toString(),
                                                     kind,
                                                     primaryChannel,
                                                     referenceChannels,
                                                     referenceWeights,
                                                     referenceSetName));
    }

    endResetModel();

    m_bFileLoaded = true;
    notifyVirtualChannelsChanged();
    return true;
}

//=============================================================================================================

bool VirtualChannelModel::saveVirtualChannels(QFile& qFile) const
{
    QFileInfo fileInfo(qFile);
    if(!fileInfo.dir().exists()) {
        qWarning() << "VirtualChannelModel: virtual-channel directory does not exist"
                   << fileInfo.absolutePath();
        return false;
    }

    if(!qFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "VirtualChannelModel: could not write virtual-channel file" << qFile.fileName();
        return false;
    }

    QJsonArray setArray;
    for(const VirtualReferenceSetDefinition& referenceSet : m_referenceSets) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), referenceSet.name);

        QJsonArray channelArray;
        for(const QString& channelName : referenceSet.channels) {
            channelArray.append(channelName);
        }
        object.insert(QStringLiteral("channels"), channelArray);
        setArray.append(object);
    }

    QJsonArray channelArray;
    for(const VirtualChannelDefinition& definition : m_virtualChannels) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), definition.name);
        object.insert(QStringLiteral("type"), kindToString(definition.kind));
        object.insert(QStringLiteral("primary_channel"), definition.primaryChannel);
        object.insert(QStringLiteral("reference_set_name"), definition.referenceSetName);

        QJsonArray referenceArray;
        for(const QString& referenceChannel : definition.referenceChannels) {
            referenceArray.append(referenceChannel);
        }
        object.insert(QStringLiteral("reference_channels"), referenceArray);

        QJsonArray weightsArray;
        for(double weight : definition.referenceWeights) {
            weightsArray.append(weight);
        }
        object.insert(QStringLiteral("reference_weights"), weightsArray);

        if(definition.kind == VirtualChannelKind::Bipolar && !definition.referenceChannels.isEmpty()) {
            object.insert(QStringLiteral("positive_channel"), definition.primaryChannel);
            object.insert(QStringLiteral("negative_channel"), definition.referenceChannels.first());
        }

        channelArray.append(object);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("version"), 3);
    rootObject.insert(QStringLiteral("reference_sets"), setArray);
    rootObject.insert(QStringLiteral("virtual_channels"), channelArray);

    const QJsonDocument document(rootObject);
    const qint64 bytesWritten = qFile.write(document.toJson(QJsonDocument::Indented));
    qFile.close();

    return bytesWritten >= 0;
}

//=============================================================================================================

void VirtualChannelModel::clearModel()
{
    beginResetModel();
    m_virtualChannels.clear();
    m_referenceSets.clear();
    m_bFileLoaded = false;
    endResetModel();

    notifyVirtualChannelsChanged();
}

//=============================================================================================================

bool VirtualChannelModel::isFileLoaded() const
{
    return m_bFileLoaded;
}

//=============================================================================================================

QString VirtualChannelModel::referenceSummaryForDefinition(const VirtualChannelDefinition& definition) const
{
    const QStringList referenceChannels = resolvedReferenceChannels(definition);
    if(definition.referenceSetName.isEmpty()) {
        return joinedReferenceChannels(referenceChannels);
    }

    if(referenceChannels.isEmpty()) {
        return QStringLiteral("@%1").arg(definition.referenceSetName);
    }

    return QStringLiteral("@%1 [%2]")
        .arg(definition.referenceSetName, joinedReferenceChannels(referenceChannels));
}

//=============================================================================================================

QString VirtualChannelModel::formulaForDefinition(const VirtualChannelDefinition& definition) const
{
    const QStringList referenceChannels = resolvedReferenceChannels(definition);
    const QVector<double> referenceWeights = resolvedReferenceWeights(definition, referenceChannels.size());

    if(referenceChannels.isEmpty()) {
        return QStringLiteral("%1 - ?").arg(definition.primaryChannel);
    }

    if(definition.kind == VirtualChannelKind::AverageReference) {
        const QString rightHandSide = definition.referenceSetName.isEmpty()
            ? QStringLiteral("avg(%1)").arg(joinedReferenceChannels(referenceChannels))
            : QStringLiteral("avg(@%1)").arg(definition.referenceSetName);
        return QStringLiteral("%1 - %2").arg(definition.primaryChannel, rightHandSide);
    }

    if(definition.kind == VirtualChannelKind::WeightedReference) {
        QStringList weightedTerms;
        weightedTerms.reserve(referenceChannels.size());
        for(int index = 0; index < referenceChannels.size(); ++index) {
            weightedTerms.append(QStringLiteral("%1*%2")
                                     .arg(formatWeight(referenceWeights.value(index, 1.0)),
                                          referenceChannels.at(index)));
        }

        return QStringLiteral("%1 - (%2)")
            .arg(definition.primaryChannel, weightedTerms.join(QStringLiteral(" + ")));
    }

    return QStringLiteral("%1 - %2")
        .arg(definition.primaryChannel, referenceChannels.first());
}

//=============================================================================================================

QStringList VirtualChannelModel::resolvedReferenceChannels(const VirtualChannelDefinition& definition) const
{
    QStringList channels = definition.referenceChannels;

    if(!definition.referenceSetName.isEmpty()) {
        const VirtualReferenceSetDefinition preset = referenceSet(definition.referenceSetName);
        if(!preset.name.isEmpty() && !preset.channels.isEmpty()) {
            channels = preset.channels;
        }
    }

    QStringList resolvedChannels;
    for(const QString& channel : channels) {
        const QString trimmed = channel.trimmed();
        if(trimmed.isEmpty() || trimmed == definition.primaryChannel || resolvedChannels.contains(trimmed)) {
            continue;
        }
        resolvedChannels.append(trimmed);
    }

    return resolvedChannels;
}

//=============================================================================================================

QVector<double> VirtualChannelModel::resolvedReferenceWeights(const VirtualChannelDefinition& definition,
                                                              int resolvedChannelCount) const
{
    QVector<double> weights;
    weights.reserve(resolvedChannelCount);

    if(definition.kind == VirtualChannelKind::WeightedReference) {
        for(int index = 0; index < resolvedChannelCount; ++index) {
            if(index < definition.referenceWeights.size()) {
                weights.append(definition.referenceWeights.at(index));
            } else {
                weights.append(1.0);
            }
        }
    } else {
        for(int index = 0; index < resolvedChannelCount; ++index) {
            weights.append(1.0);
        }
    }

    return weights;
}

//=============================================================================================================

VirtualReferenceSetDefinition VirtualChannelModel::normalizeReferenceSet(const QString& name,
                                                                         const QStringList& channels) const
{
    VirtualReferenceSetDefinition definition;
    definition.name = name.trimmed();

    for(const QString& channel : channels) {
        const QString trimmed = channel.trimmed();
        if(trimmed.isEmpty() || definition.channels.contains(trimmed)) {
            continue;
        }
        definition.channels.append(trimmed);
    }

    return definition;
}

//=============================================================================================================

VirtualChannelDefinition VirtualChannelModel::normalizeDefinition(const QString& name,
                                                                  VirtualChannelKind kind,
                                                                  const QString& primaryChannel,
                                                                  const QStringList& referenceChannels,
                                                                  const QVector<double>& referenceWeights,
                                                                  const QString& referenceSetName) const
{
    VirtualChannelDefinition definition;
    definition.kind = kind;
    definition.primaryChannel = primaryChannel.trimmed();
    definition.referenceSetName = referenceSetName.trimmed();

    QStringList effectiveReferenceChannels = referenceChannels;
    if(effectiveReferenceChannels.isEmpty() && !definition.referenceSetName.isEmpty()) {
        effectiveReferenceChannels = referenceSet(definition.referenceSetName).channels;
    }

    for(int index = 0; index < effectiveReferenceChannels.size(); ++index) {
        const QString trimmed = effectiveReferenceChannels.at(index).trimmed();
        if(trimmed.isEmpty()
           || trimmed == definition.primaryChannel
           || definition.referenceChannels.contains(trimmed)) {
            continue;
        }

        definition.referenceChannels.append(trimmed);
        if(kind == VirtualChannelKind::WeightedReference) {
            definition.referenceWeights.append(index < referenceWeights.size()
                                               ? referenceWeights.at(index)
                                               : 1.0);
        }
    }

    definition.name = name.trimmed();
    if(definition.name.isEmpty()) {
        if(definition.kind == VirtualChannelKind::AverageReference) {
            definition.name = definition.referenceSetName.isEmpty()
                ? QStringLiteral("%1-avgref").arg(definition.primaryChannel)
                : QStringLiteral("%1-avgref-%2").arg(definition.primaryChannel, definition.referenceSetName);
        } else if(definition.kind == VirtualChannelKind::WeightedReference) {
            definition.name = definition.referenceSetName.isEmpty()
                ? QStringLiteral("%1-wref").arg(definition.primaryChannel)
                : QStringLiteral("%1-wref-%2").arg(definition.primaryChannel, definition.referenceSetName);
        } else {
            definition.name = QStringLiteral("%1-%2")
                .arg(definition.primaryChannel,
                     definition.referenceChannels.isEmpty()
                         ? QStringLiteral("?")
                         : definition.referenceChannels.first());
        }
    }

    return definition;
}

//=============================================================================================================

void VirtualChannelModel::notifyVirtualChannelsChanged()
{
    emit virtualChannelsChanged();
}
