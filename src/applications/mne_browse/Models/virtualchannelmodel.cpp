//=============================================================================================================
/**
 * @file     virtualchannelmodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
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
        case VirtualChannelKind::Bipolar:
        default:
            return QStringLiteral("bipolar");
    }
}

VirtualChannelKind stringToKind(const QString& text)
{
    return text.trimmed().compare(QStringLiteral("average_reference"), Qt::CaseInsensitive) == 0
        ? VirtualChannelKind::AverageReference
        : VirtualChannelKind::Bipolar;
}

QString kindToDisplayString(VirtualChannelKind kind)
{
    return kind == VirtualChannelKind::AverageReference
        ? QStringLiteral("Average Reference")
        : QStringLiteral("Bipolar");
}

QString joinedReferenceChannels(const QStringList& referenceChannels)
{
    return referenceChannels.join(QStringLiteral(", "));
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
            return joinedReferenceChannels(definition.referenceChannels);
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
                                     definition.referenceChannels);

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
                                           const QStringList& referenceChannels)
{
    const VirtualChannelDefinition definition =
        normalizeDefinition(name, kind, primaryChannel, referenceChannels);

    const int row = m_virtualChannels.size();
    beginInsertRows(QModelIndex(), row, row);
    m_virtualChannels.append(definition);
    endInsertRows();

    notifyVirtualChannelsChanged();
    return row;
}

//=============================================================================================================

QVector<VirtualChannelDefinition> VirtualChannelModel::virtualChannels() const
{
    return m_virtualChannels;
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

    const QJsonArray channelArray = document.object().value(QStringLiteral("virtual_channels")).toArray();

    beginResetModel();
    m_virtualChannels.clear();

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

        VirtualChannelKind kind = stringToKind(object.value(QStringLiteral("type")).toString());
        QString primaryChannel = object.value(QStringLiteral("primary_channel")).toString();

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
                                                     referenceChannels));
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

    QJsonArray channelArray;
    for(const VirtualChannelDefinition& definition : m_virtualChannels) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), definition.name);
        object.insert(QStringLiteral("type"), kindToString(definition.kind));
        object.insert(QStringLiteral("primary_channel"), definition.primaryChannel);

        QJsonArray referenceArray;
        for(const QString& referenceChannel : definition.referenceChannels) {
            referenceArray.append(referenceChannel);
        }
        object.insert(QStringLiteral("reference_channels"), referenceArray);

        if(definition.kind == VirtualChannelKind::Bipolar && !definition.referenceChannels.isEmpty()) {
            object.insert(QStringLiteral("positive_channel"), definition.primaryChannel);
            object.insert(QStringLiteral("negative_channel"), definition.referenceChannels.first());
        }

        channelArray.append(object);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("version"), 2);
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

QString VirtualChannelModel::formulaForDefinition(const VirtualChannelDefinition& definition) const
{
    if(definition.kind == VirtualChannelKind::AverageReference) {
        return QStringLiteral("%1 - avg(%2)")
            .arg(definition.primaryChannel, joinedReferenceChannels(definition.referenceChannels));
    }

    return QStringLiteral("%1 - %2")
        .arg(definition.primaryChannel,
             definition.referenceChannels.isEmpty() ? QStringLiteral("?") : definition.referenceChannels.first());
}

//=============================================================================================================

VirtualChannelDefinition VirtualChannelModel::normalizeDefinition(const QString& name,
                                                                  VirtualChannelKind kind,
                                                                  const QString& primaryChannel,
                                                                  const QStringList& referenceChannels) const
{
    VirtualChannelDefinition definition;
    definition.kind = kind;
    definition.primaryChannel = primaryChannel.trimmed();

    for(const QString& referenceChannel : referenceChannels) {
        const QString trimmed = referenceChannel.trimmed();
        if(trimmed.isEmpty() || trimmed == definition.primaryChannel || definition.referenceChannels.contains(trimmed)) {
            continue;
        }
        definition.referenceChannels.append(trimmed);
    }

    definition.name = name.trimmed();
    if(definition.name.isEmpty()) {
        if(definition.kind == VirtualChannelKind::AverageReference) {
            definition.name = QStringLiteral("%1-avgref").arg(definition.primaryChannel);
        } else {
            definition.name = QStringLiteral("%1-%2")
                .arg(definition.primaryChannel,
                     definition.referenceChannels.isEmpty() ? QStringLiteral("?") : definition.referenceChannels.first());
        }
    }

    return definition;
}

//=============================================================================================================

void VirtualChannelModel::notifyVirtualChannelsChanged()
{
    emit virtualChannelsChanged();
}
