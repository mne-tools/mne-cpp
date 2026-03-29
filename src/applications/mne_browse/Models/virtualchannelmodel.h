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
#include <QStringList>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

enum class VirtualChannelKind {
    Bipolar = 0,
    AverageReference,
    WeightedReference
};

struct VirtualReferenceSetDefinition {
    QString     name;
    QStringList channels;
};

struct VirtualChannelDefinition {
    QString            name;
    VirtualChannelKind kind = VirtualChannelKind::Bipolar;
    QString            primaryChannel;
    QStringList        referenceChannels;
    QVector<double>    referenceWeights;
    QString            referenceSetName;
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
                          VirtualChannelKind kind,
                          const QString& primaryChannel,
                          const QStringList& referenceChannels,
                          const QVector<double>& referenceWeights = {},
                          const QString& referenceSetName = QString());

    int addReferenceSet(const QString& name, const QStringList& channels);
    bool removeReferenceSet(const QString& name);

    QVector<VirtualChannelDefinition> virtualChannels() const;
    QVector<VirtualReferenceSetDefinition> referenceSets() const;
    QStringList referenceSetNames() const;
    VirtualReferenceSetDefinition referenceSet(const QString& name) const;

    bool loadVirtualChannels(QFile& qFile);
    bool saveVirtualChannels(QFile& qFile) const;

    void clearModel();
    bool isFileLoaded() const;

signals:
    void virtualChannelsChanged();

private:
    QString referenceSummaryForDefinition(const VirtualChannelDefinition& definition) const;
    QString formulaForDefinition(const VirtualChannelDefinition& definition) const;
    QStringList resolvedReferenceChannels(const VirtualChannelDefinition& definition) const;
    QVector<double> resolvedReferenceWeights(const VirtualChannelDefinition& definition,
                                             int resolvedChannelCount) const;
    VirtualReferenceSetDefinition normalizeReferenceSet(const QString& name,
                                                        const QStringList& channels) const;
    VirtualChannelDefinition normalizeDefinition(const QString& name,
                                                 VirtualChannelKind kind,
                                                 const QString& primaryChannel,
                                                 const QStringList& referenceChannels,
                                                 const QVector<double>& referenceWeights = {},
                                                 const QString& referenceSetName = QString()) const;
    void notifyVirtualChannelsChanged();

    QVector<VirtualChannelDefinition>      m_virtualChannels;
    QVector<VirtualReferenceSetDefinition> m_referenceSets;
    bool                                   m_bFileLoaded = false;
};

} // namespace MNEBROWSE

#endif // VIRTUALCHANNELMODEL_H
