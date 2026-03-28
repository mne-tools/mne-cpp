//=============================================================================================================
/**
 * @file     virtualchannelmodel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Table model for browser-level virtual bipolar channels.
 */

#ifndef VIRTUALCHANNELMODEL_H
#define VIRTUALCHANNELMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QFile>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

struct VirtualChannelDefinition {
    QString name;
    QString positiveChannel;
    QString negativeChannel;
};

class VirtualChannelModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit VirtualChannelModel(QObject *parent = nullptr);
    ~VirtualChannelModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;

    int addVirtualChannel(const QString& name,
                          const QString& positiveChannel,
                          const QString& negativeChannel);

    QVector<VirtualChannelDefinition> virtualChannels() const;

    bool loadVirtualChannels(QFile& qFile);
    bool saveVirtualChannels(QFile& qFile) const;

    void clearModel();
    bool isFileLoaded() const;

signals:
    void virtualChannelsChanged();

private:
    VirtualChannelDefinition normalizeDefinition(const QString& name,
                                                 const QString& positiveChannel,
                                                 const QString& negativeChannel) const;
    void notifyVirtualChannelsChanged();

    QVector<VirtualChannelDefinition> m_virtualChannels;
    bool                              m_bFileLoaded = false;
};

} // namespace MNEBROWSE

#endif // VIRTUALCHANNELMODEL_H
