//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     epochmodel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  2.1.0
 * @brief    Declaration of the EpochModel class.
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

//=============================================================================================================
/**
 * @brief Table model that exposes epoch review state before evoked averaging.
 */
class EpochModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs an empty epoch review model.
     *
     * @param[in] parent    Parent QObject.
     */
    explicit EpochModel(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the epoch review model.
     */
    ~EpochModel() override;

    //=========================================================================================================
    /**
     * Returns the number of epoch rows currently shown.
     *
     * @param[in] parent    Parent index supplied by Qt.
     * @return Number of reviewable epochs.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the number of columns exposed by the epoch review table.
     *
     * @param[in] parent    Parent index supplied by Qt.
     * @return Number of model columns.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns header metadata for the epoch review table.
     *
     * @param[in] section       Header section index.
     * @param[in] orientation   Header orientation.
     * @param[in] role          Requested Qt role.
     * @return Header value for the requested role.
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Returns display and state data for one epoch-review cell.
     *
     * @param[in] index     Requested model index.
     * @param[in] role      Requested Qt role.
     * @return Cell data for the requested role.
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Updates the manual inclusion state of one epoch row.
     *
     * @param[in] index     Target model index.
     * @param[in] value     New value.
     * @param[in] role      Edit role.
     * @return True on success.
     */
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    //=========================================================================================================
    /**
     * Returns the supported Qt item flags for one epoch row.
     *
     * @param[in] index     Requested model index.
     * @return Supported flags.
     */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Replaces the model contents with epochs produced by one averaging run.
     *
     * @param[in] epochLists    Epoch lists grouped by selected event code.
     * @param[in] eventCodes    Event codes corresponding to epochLists.
     * @param[in] firstSample   First raw sample of the recording.
     * @param[in] sfreq         Recording sampling frequency in Hz.
     */
    void setEpochs(const QList<MNELIB::MNEEpochDataList>& epochLists,
                   const QList<int>& eventCodes,
                   int firstSample,
                   float sfreq);

    //=========================================================================================================
    /**
     * Clears all epoch review rows.
     */
    void clearModel();

    //=========================================================================================================
    /**
     * Controls whether automatically rejected epochs are still excluded from averaging.
     *
     * @param[in] respectAutoRejects  True to keep honoring automatic rejections.
     */
    void setRespectAutoRejects(bool respectAutoRejects);

    //=========================================================================================================
    /**
     * Returns whether automatic rejections are currently honored.
     *
     * @return True if auto-rejected epochs remain excluded.
     */
    bool respectAutoRejects() const;

    //=========================================================================================================
    /**
     * Clears all manual include/exclude decisions while keeping automatic rejection state.
     */
    void resetManualExclusions();

    //=========================================================================================================
    /**
     * Returns the absolute sample position of one epoch trigger.
     *
     * @param[in] row   Epoch row index.
     * @return Absolute trigger sample, or 0 if the row is invalid.
     */
    int sampleAt(int row) const;

    //=========================================================================================================
    /**
     * Builds a compact summary string for the current review state.
     *
     * @return Human-readable summary text.
     */
    QString summaryText() const;

    //=========================================================================================================
    /**
     * Returns peak-to-peak amplitudes (max across channels) for each included epoch.
     *
     * @return Vector of PTP values for included epochs.
     */
    QVector<double> ptpAmplitudes() const;

signals:
    //=========================================================================================================
    /**
     * Emitted whenever epoch inclusion state changes.
     */
    void epochsChanged();

private:
    //=========================================================================================================
    /**
     * @brief Cached information for one reviewable epoch row.
     */
    struct EpochEntry
    {
        MNELIB::MNEEpochData::SPtr epoch;   /**< Backing epoch object with reject/exclude state. */
        int eventCode = 0;                  /**< Event code that created this epoch. */
        int absoluteSample = 0;             /**< Absolute trigger sample in the raw file. */
        int sample = 0;                     /**< Display sample relative to the loaded raw file. */
        double timeSeconds = 0.0;           /**< Trigger time in seconds relative to file start. */
    };

    //=========================================================================================================
    /**
     * Returns whether the epoch should currently contribute to averaging.
     *
     * @param[in] entry  Epoch entry to inspect.
     * @return True if the epoch is included.
     */
    bool epochIsIncluded(const EpochEntry& entry) const;

    //=========================================================================================================
    /**
     * Builds the status text shown for one epoch entry.
     *
     * @param[in] entry  Epoch entry to summarize.
     * @return Human-readable status string.
     */
    QString epochStatusText(const EpochEntry& entry) const;

    QList<EpochEntry> m_entries;              /**< Flattened list of all reviewable epochs. */
    bool m_bRespectAutoRejects = true;        /**< True if automatic rejection remains active. */
};

} // namespace MNEBROWSE

#endif // EPOCHMODEL_H
