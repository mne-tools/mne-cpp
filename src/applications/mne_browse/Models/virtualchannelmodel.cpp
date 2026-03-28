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
    return 4;
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
                return QStringLiteral("Positive");
            case 2:
                return QStringLiteral("Negative");
            case 3:
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
            return definition.positiveChannel;
        case 2:
            return definition.negativeChannel;
        case 3:
            return QStringLiteral("%1 - %2")
                .arg(definition.positiveChannel, definition.negativeChannel);
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
                                     definition.positiveChannel,
                                     definition.negativeChannel);

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
    for(int i = 0; i < rows; ++i) {
        m_virtualChannels.removeAt(position);
    }
    endRemoveRows();

    notifyVirtualChannelsChanged();
    return true;
}

//=============================================================================================================

int VirtualChannelModel::addVirtualChannel(const QString& name,
                                           const QString& positiveChannel,
                                           const QString& negativeChannel)
{
    const VirtualChannelDefinition definition =
        normalizeDefinition(name, positiveChannel, negativeChannel);

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

        m_virtualChannels.append(normalizeDefinition(
            object.value(QStringLiteral("name")).toString(),
            object.value(QStringLiteral("positive_channel")).toString(),
            object.value(QStringLiteral("negative_channel")).toString()));
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
        object.insert(QStringLiteral("positive_channel"), definition.positiveChannel);
        object.insert(QStringLiteral("negative_channel"), definition.negativeChannel);
        channelArray.append(object);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("version"), 1);
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

VirtualChannelDefinition VirtualChannelModel::normalizeDefinition(const QString& name,
                                                                  const QString& positiveChannel,
                                                                  const QString& negativeChannel) const
{
    VirtualChannelDefinition definition;
    definition.positiveChannel = positiveChannel.trimmed();
    definition.negativeChannel = negativeChannel.trimmed();
    definition.name = name.trimmed();

    if(definition.name.isEmpty()) {
        definition.name = QStringLiteral("%1-%2")
            .arg(definition.positiveChannel, definition.negativeChannel);
    }

    return definition;
}

//=============================================================================================================

void VirtualChannelModel::notifyVirtualChannelsChanged()
{
    emit virtualChannelsChanged();
}
