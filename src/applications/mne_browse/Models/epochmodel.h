//=============================================================================================================
/**
 * @file     epochmodel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Table model used to review detected epochs before averaging.
 */

#ifndef EPOCHMODEL_H
#define EPOCHMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_epoch_data_list.h>

#include <QAbstractTableModel>

namespace MNEBROWSE
{

class EpochModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EpochModel(QObject *parent = nullptr);
    ~EpochModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setEpochs(const QList<MNELIB::MNEEpochDataList>& epochLists,
                   const QList<int>& eventCodes,
                   int firstSample,
                   float sfreq);
    void clearModel();

    void setRespectAutoRejects(bool respectAutoRejects);
    bool respectAutoRejects() const;

    void resetManualExclusions();

    int sampleAt(int row) const;
    QString summaryText() const;

signals:
    void epochsChanged();

private:
    struct EpochEntry
    {
        MNELIB::MNEEpochData::SPtr epoch;
        int eventCode = 0;
        int absoluteSample = 0;
        int sample = 0;
        double timeSeconds = 0.0;
    };

    bool epochIsIncluded(const EpochEntry& entry) const;
    QString epochStatusText(const EpochEntry& entry) const;

    QList<EpochEntry> m_entries;
    bool m_bRespectAutoRejects = true;
};

} // namespace MNEBROWSE

#endif // EPOCHMODEL_H
