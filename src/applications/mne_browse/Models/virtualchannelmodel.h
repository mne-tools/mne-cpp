//=============================================================================================================
/**
 * @file     virtualchannelmodel.h
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
 * @brief    Declaration of the VirtualChannelModel class.
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

//=============================================================================================================
/**
 * @brief Supported virtual-channel derivation types.
 */
enum class VirtualChannelKind {
    Bipolar = 0,          /**< Classic source minus reference derivation. */
    AverageReference,     /**< Source minus the arithmetic mean of all references. */
    WeightedReference     /**< Source minus a weighted sum of references. */
};

//=============================================================================================================
/**
 * @brief Named collection of reference channels that can be reused by multiple virtual channels.
 */
struct VirtualReferenceSetDefinition {
    QString     name;       /**< User-visible name of the reference set. */
    QStringList channels;   /**< Ordered list of reference channel names. */
};

//=============================================================================================================
/**
 * @brief Serializable description of one derived browser channel.
 */
struct VirtualChannelDefinition {
    QString            name;                /**< User-visible name of the derived channel. */
    VirtualChannelKind kind = VirtualChannelKind::Bipolar; /**< Derivation type. */
    QString            primaryChannel;      /**< Source channel that is shown as the main signal. */
    QStringList        referenceChannels;   /**< Explicit reference channels used by the definition. */
    QVector<double>    referenceWeights;    /**< Optional per-reference weights for weighted derivations. */
    QString            referenceSetName;    /**< Optional named reference set resolved at runtime. */
};

//=============================================================================================================
/**
 * @brief Table model that stores persistent virtual-channel definitions for the raw browser.
 *
 * The model owns the user-defined derivation metadata only. The actual sample computation is performed
 * later in the fast FIFF block loading path when visible data blocks are requested.
 */
class VirtualChannelModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs an empty virtual-channel model.
     *
     * @param[in] parent    Parent QObject.
     */
    explicit VirtualChannelModel(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the virtual-channel model.
     */
    ~VirtualChannelModel() override;

    //=========================================================================================================
    /**
     * Returns the number of stored virtual-channel definitions.
     *
     * @param[in] parent    Parent index supplied by Qt.
     * @return Number of virtual channels shown in the table.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the number of columns exposed by the table model.
     *
     * @param[in] parent    Parent index supplied by Qt.
     * @return Number of virtual-channel columns.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns header metadata for the virtual-channel table.
     *
     * @param[in] section       Header section index.
     * @param[in] orientation   Header orientation.
     * @param[in] role          Requested Qt role.
     * @return Header value for the requested role.
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    //=========================================================================================================
    /**
     * Returns display, edit, and decoration data for one table cell.
     *
     * @param[in] index     Requested model index.
     * @param[in] role      Requested Qt role.
     * @return Cell data for the requested role.
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Returns the Qt item flags for one virtual-channel row.
     *
     * @param[in] index     Requested model index.
     * @return Supported item flags.
     */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Updates one editable cell in the virtual-channel table.
     *
     * @param[in] index     Target model index.
     * @param[in] value     New value.
     * @param[in] role      Edit role.
     * @return True on success.
     */
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    //=========================================================================================================
    /**
     * Removes one or more virtual-channel definitions.
     *
     * @param[in] position  First row to remove.
     * @param[in] rows      Number of rows to remove.
     * @param[in] parent    Parent index supplied by Qt.
     * @return True on success.
     */
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;

    //=========================================================================================================
    /**
     * Adds a new virtual-channel definition to the model.
     *
     * @param[in] name               Display name of the new virtual channel.
     * @param[in] kind               Derivation type.
     * @param[in] primaryChannel     Primary source channel.
     * @param[in] referenceChannels  Explicit reference channels.
     * @param[in] referenceWeights   Optional weights matching the resolved reference channels.
     * @param[in] referenceSetName   Optional reusable reference-set name.
     * @return Row index of the newly inserted definition.
     */
    int addVirtualChannel(const QString& name,
                          VirtualChannelKind kind,
                          const QString& primaryChannel,
                          const QStringList& referenceChannels,
                          const QVector<double>& referenceWeights = {},
                          const QString& referenceSetName = QString());

    //=========================================================================================================
    /**
     * Adds or replaces a named reusable reference set.
     *
     * @param[in] name      Name of the reference set.
     * @param[in] channels  Channels contained in the set.
     * @return Row index of the stored set.
     */
    int addReferenceSet(const QString& name, const QStringList& channels);

    //=========================================================================================================
    /**
     * Removes a named reusable reference set.
     *
     * @param[in] name  Name of the reference set.
     * @return True if a set with the given name was removed.
     */
    bool removeReferenceSet(const QString& name);

    //=========================================================================================================
    /**
     * Returns all stored virtual-channel definitions.
     *
     * @return Virtual-channel definition list.
     */
    QVector<VirtualChannelDefinition> virtualChannels() const;

    //=========================================================================================================
    /**
     * Returns all stored reusable reference sets.
     *
     * @return Reference-set definitions.
     */
    QVector<VirtualReferenceSetDefinition> referenceSets() const;

    //=========================================================================================================
    /**
     * Returns the names of all available reference sets.
     *
     * @return Reference-set names.
     */
    QStringList referenceSetNames() const;

    //=========================================================================================================
    /**
     * Returns one named reference set.
     *
     * @param[in] name  Name of the reference set.
     * @return Matching reference-set definition, or an empty one if not found.
     */
    VirtualReferenceSetDefinition referenceSet(const QString& name) const;

    //=========================================================================================================
    /**
     * Loads virtual-channel definitions from a sidecar file.
     *
     * @param[in,out] qFile  Open file handle to read from.
     * @return True on success.
     */
    bool loadVirtualChannels(QFile& qFile);

    //=========================================================================================================
    /**
     * Saves virtual-channel definitions to a sidecar file.
     *
     * @param[in,out] qFile  Open file handle to write to.
     * @return True on success.
     */
    bool saveVirtualChannels(QFile& qFile) const;

    //=========================================================================================================
    /**
     * Clears all definitions and resets the loaded-file state.
     */
    void clearModel();

    //=========================================================================================================
    /**
     * Returns true if the current definitions originate from a file load.
     *
     * @return True if a virtual-channel sidecar file has been loaded.
     */
    bool isFileLoaded() const;

signals:
    //=========================================================================================================
    /**
     * Emitted whenever the set of virtual channels or reusable reference sets changes.
     */
    void virtualChannelsChanged();

private:
    //=========================================================================================================
    /**
     * Creates the compact human-readable reference summary shown in the table.
     *
     * @param[in] definition  Virtual-channel definition to summarize.
     * @return Reference summary string.
     */
    QString referenceSummaryForDefinition(const VirtualChannelDefinition& definition) const;

    //=========================================================================================================
    /**
     * Creates the formula string shown in the table view.
     *
     * @param[in] definition  Virtual-channel definition to summarize.
     * @return Human-readable derivation formula.
     */
    QString formulaForDefinition(const VirtualChannelDefinition& definition) const;

    //=========================================================================================================
    /**
     * Resolves the effective list of reference channels for one definition.
     *
     * @param[in] definition  Definition to resolve.
     * @return Ordered list of reference channel names.
     */
    QStringList resolvedReferenceChannels(const VirtualChannelDefinition& definition) const;

    //=========================================================================================================
    /**
     * Resolves the effective weights for one definition.
     *
     * @param[in] definition            Definition to resolve.
     * @param[in] resolvedChannelCount  Number of resolved reference channels.
     * @return Weight vector matching the resolved channel list.
     */
    QVector<double> resolvedReferenceWeights(const VirtualChannelDefinition& definition,
                                             int resolvedChannelCount) const;

    //=========================================================================================================
    /**
     * Normalizes a reusable reference-set definition before storage.
     *
     * @param[in] name      Reference-set name.
     * @param[in] channels  Proposed channel list.
     * @return Normalized reference-set definition.
     */
    VirtualReferenceSetDefinition normalizeReferenceSet(const QString& name,
                                                        const QStringList& channels) const;

    //=========================================================================================================
    /**
     * Normalizes a virtual-channel definition before insertion.
     *
     * @param[in] name               Virtual-channel name.
     * @param[in] kind               Derivation type.
     * @param[in] primaryChannel     Primary source channel.
     * @param[in] referenceChannels  Explicit reference channels.
     * @param[in] referenceWeights   Optional reference weights.
     * @param[in] referenceSetName   Optional reusable reference-set name.
     * @return Normalized virtual-channel definition.
     */
    VirtualChannelDefinition normalizeDefinition(const QString& name,
                                                 VirtualChannelKind kind,
                                                 const QString& primaryChannel,
                                                 const QStringList& referenceChannels,
                                                 const QVector<double>& referenceWeights = {},
                                                 const QString& referenceSetName = QString()) const;

    //=========================================================================================================
    /**
     * Emits the consolidated change signal after model updates.
     */
    void notifyVirtualChannelsChanged();

    QVector<VirtualChannelDefinition>      m_virtualChannels;   /**< Stored virtual-channel definitions. */
    QVector<VirtualReferenceSetDefinition> m_referenceSets;     /**< Stored reusable reference sets. */
    bool                                   m_bFileLoaded = false; /**< True if the model state came from disk. */
};

} // namespace MNEBROWSE

#endif // VIRTUALCHANNELMODEL_H
