//=============================================================================================================
/**
 * @file     annotationmodel.h
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
 * @brief    Declaration of the AnnotationModel class.
 */

#ifndef ANNOTATIONMODEL_H
#define ANNOTATIONMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QColor>
#include <QFile>
#include <QPair>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * @brief Lightweight span information used by the raw browser overlay renderer.
 */
struct AnnotationSpanData {
    int     startSample = 0;         /**< Inclusive span start sample. */
    int     endSample   = 0;         /**< Inclusive span end sample. */
    QColor  color;                   /**< Display color derived from the annotation label. */
    QString label;                   /**< Short label rendered into the browser overlay. */
    QString comment;                 /**< Optional free-form annotation comment. */
    QStringList channelNames;        /**< Optional channel list the annotation applies to. */
};

//=============================================================================================================
/**
 * @brief Table model that stores editable raw-browser annotations and sidecar import/export state.
 */
class AnnotationModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs an empty annotation model.
     *
     * @param[in] parent    Parent QObject.
     */
    explicit AnnotationModel(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the annotation model.
     */
    ~AnnotationModel() override;

    //=========================================================================================================
    /**
     * Returns the number of stored annotations.
     *
     * @param[in] parent    Parent index supplied by Qt.
     * @return Number of annotations shown in the table.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the number of annotation table columns.
     *
     * @param[in] parent    Parent index supplied by Qt.
     * @return Number of model columns.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns header metadata for the annotation table.
     *
     * @param[in] section       Header section index.
     * @param[in] orientation   Header orientation.
     * @param[in] role          Requested Qt role.
     * @return Header value for the requested role.
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    //=========================================================================================================
    /**
     * Returns display, edit, and decoration data for one annotation cell.
     *
     * @param[in] index     Requested model index.
     * @param[in] role      Requested Qt role.
     * @return Cell data for the requested role.
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Returns the supported Qt item flags for one annotation row.
     *
     * @param[in] index     Requested model index.
     * @return Supported item flags.
     */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Updates an editable annotation field.
     *
     * @param[in] index     Target model index.
     * @param[in] value     New value.
     * @param[in] role      Edit role.
     * @return True on success.
     */
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    //=========================================================================================================
    /**
     * Removes one or more annotation rows.
     *
     * @param[in] position  First row to remove.
     * @param[in] rows      Number of rows to remove.
     * @param[in] parent    Parent index supplied by Qt.
     * @return True on success.
     */
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;

    //=========================================================================================================
    /**
     * Sets the FIFF measurement information used for sample/time conversion.
     *
     * @param[in] pFiffInfo  Measurement info of the currently loaded raw file.
     */
    void setFiffInfo(const FIFFLIB::FiffInfo::SPtr& pFiffInfo);

    //=========================================================================================================
    /**
     * Sets the raw file sample bounds used for validation and conversion.
     *
     * @param[in] firstSample  First sample of the raw recording.
     * @param[in] lastSample   Last sample of the raw recording.
     */
    void setFirstLastSample(int firstSample, int lastSample);

    //=========================================================================================================
    /**
     * Adds one annotation row to the model.
     *
     * @param[in] startSample   Inclusive annotation start sample.
     * @param[in] endSample     Inclusive annotation end sample.
     * @param[in] description   Annotation label or description.
     * @param[in] channelNames  Optional affected channel names.
     * @param[in] comment       Optional free-form comment.
     * @return Row index of the new annotation.
     */
    int addAnnotation(int startSample,
                      int endSample,
                      const QString& description,
                      const QStringList& channelNames = QStringList(),
                      const QString& comment = QString());

    //=========================================================================================================
    /**
     * Updates the start or end boundary of an annotation by absolute sample index.
     *
     * @param[in] row           Annotation row index.
     * @param[in] isStart       True to update startSample, false for endSample.
     * @param[in] absoluteSample New absolute sample value.
     * @return True on success.
     */
    bool updateAnnotationBoundary(int row, bool isStart, int absoluteSample);

    //=========================================================================================================
    /**
     * Returns the sample range of one annotation row.
     *
     * @param[in] row   Annotation row index.
     * @return Pair of start and end sample.
     */
    QPair<int, int> getSampleRange(int row) const;

    //=========================================================================================================
    /**
     * Returns annotation spans formatted for the browser overlay.
     *
     * @return Display-ready span list.
     */
    QVector<AnnotationSpanData> getAnnotationSpans() const;

    //=========================================================================================================
    /**
     * Loads annotations from JSON, FIF, CSV, or TXT.
     *
     * @param[in,out] qFile  Open file handle to read from.
     * @return True on success.
     */
    bool loadAnnotationData(QFile& qFile);

    //=========================================================================================================
    /**
     * Saves annotations to JSON, FIF, CSV, or TXT.
     *
     * @param[in,out] qFile  Open file handle to write to.
     * @return True on success.
     */
    bool saveAnnotationData(QFile& qFile) const;

    //=========================================================================================================
    /**
     * Clears all annotations and resets file-backed state.
     */
    void clearModel();

    //=========================================================================================================
    /**
     * Returns true if the current annotations originate from a file load.
     *
     * @return True if annotations were loaded from disk.
     */
    bool isFileLoaded() const;

signals:
    //=========================================================================================================
    /**
     * Emitted whenever the annotation list changes.
     */
    void annotationsChanged();

private:
    //=========================================================================================================
    /**
     * @brief Internal editable representation of one annotation row.
     */
    struct AnnotationEntry {
        int     startSample = 0;         /**< Inclusive annotation start sample. */
        int     endSample   = 0;         /**< Inclusive annotation end sample. */
        QString description;             /**< Annotation label or description. */
        QStringList channelNames;        /**< Optional affected channels. */
        QString comment;                 /**< Optional free-form comment. */
        QVariantMap extras;              /**< Format-specific metadata retained for round-tripping. */
    };

    //=========================================================================================================
    /**
     * Loads annotations from the legacy JSON sidecar format.
     *
     * @param[in,out] qFile  Open file handle to read from.
     * @return True on success.
     */
    bool loadAnnotationJson(QFile& qFile);

    //=========================================================================================================
    /**
     * Loads annotations from an MNE annotation FIF file.
     *
     * @param[in,out] qFile  Open file handle to read from.
     * @return True on success.
     */
    bool loadAnnotationFif(QFile& qFile);

    //=========================================================================================================
    /**
     * Loads annotations from an MNE-compatible CSV file.
     *
     * @param[in,out] qFile  Open file handle to read from.
     * @return True on success.
     */
    bool loadAnnotationCsv(QFile& qFile);

    //=========================================================================================================
    /**
     * Loads annotations from a text export file.
     *
     * @param[in,out] qFile  Open file handle to read from.
     * @return True on success.
     */
    bool loadAnnotationTxt(QFile& qFile);

    //=========================================================================================================
    /**
     * Saves annotations to the legacy JSON sidecar format.
     *
     * @param[in,out] qFile  Open file handle to write to.
     * @return True on success.
     */
    bool saveAnnotationJson(QFile& qFile) const;

    //=========================================================================================================
    /**
     * Saves annotations to an MNE annotation FIF file.
     *
     * @param[in,out] qFile  Open file handle to write to.
     * @return True on success.
     */
    bool saveAnnotationFif(QFile& qFile) const;

    //=========================================================================================================
    /**
     * Saves annotations to an MNE-compatible CSV file.
     *
     * @param[in,out] qFile  Open file handle to write to.
     * @return True on success.
     */
    bool saveAnnotationCsv(QFile& qFile) const;

    //=========================================================================================================
    /**
     * Saves annotations to a plain text export file.
     *
     * @param[in,out] qFile  Open file handle to write to.
     * @return True on success.
     */
    bool saveAnnotationTxt(QFile& qFile) const;

    //=========================================================================================================
    /**
     * Normalizes a user-supplied annotation before insertion or storage.
     *
     * @param[in] startSample   Inclusive annotation start sample.
     * @param[in] endSample     Inclusive annotation end sample.
     * @param[in] description   Annotation label or description.
     * @param[in] channelNames  Optional affected channel names.
     * @param[in] comment       Optional free-form comment.
     * @param[in] extras        Additional round-trip metadata.
     * @return Normalized annotation entry.
     */
    AnnotationEntry normalizeEntry(int startSample,
                                   int endSample,
                                   const QString& description,
                                   const QStringList& channelNames = QStringList(),
                                   const QString& comment = QString(),
                                   const QVariantMap& extras = QVariantMap()) const;

    //=========================================================================================================
    /**
     * Converts a raw-file sample index into an onset value in seconds.
     *
     * @param[in] sample   Sample index to convert.
     * @return Onset time in seconds.
     */
    double sampleToOnsetSeconds(int sample) const;

    //=========================================================================================================
    /**
     * Converts an onset time in seconds into a raw-file sample index.
     *
     * @param[in] onsetSeconds  Onset time in seconds.
     * @return Closest raw-file sample index.
     */
    int onsetSecondsToSample(double onsetSeconds) const;

    //=========================================================================================================
    /**
     * Converts a duration in seconds into a sample count.
     *
     * @param[in] durationSeconds  Duration in seconds.
     * @return Duration in samples.
     */
    int durationSecondsToSamples(double durationSeconds) const;

    //=========================================================================================================
    /**
     * Returns the measurement start time in microseconds since the Unix epoch.
     *
     * @param[out] ok  Set to true if the timestamp could be derived.
     * @return Measurement start time in microseconds since epoch.
     */
    qint64 measurementStartUsecsSinceEpoch(bool *ok = nullptr) const;

    //=========================================================================================================
    /**
     * Sorts all annotations into ascending sample order.
     */
    void sortEntries();

    //=========================================================================================================
    /**
     * Returns the display color associated with one annotation description.
     *
     * @param[in] description   Annotation label.
     * @return Display color used in the browser overlay and manager.
     */
    QColor colorForLabel(const QString& description) const;

    //=========================================================================================================
    /**
     * Emits the consolidated change signal after model updates.
     */
    void notifyAnnotationsChanged();

    QVector<AnnotationEntry>  m_annotations;   /**< Editable annotation list. */
    FIFFLIB::FiffInfo::SPtr   m_pFiffInfo;     /**< Measurement info used for sample/time conversion. */
    int                       m_iFirstSample = 0; /**< First sample of the loaded raw file. */
    int                       m_iLastSample  = 0; /**< Last sample of the loaded raw file. */
    bool                      m_bFileLoaded  = false; /**< True if annotations were loaded from disk. */
};

} // namespace MNEBROWSE

#endif // ANNOTATIONMODEL_H
