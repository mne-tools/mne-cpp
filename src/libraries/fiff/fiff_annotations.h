//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_annotations.h
 * @since April 2026
 * @brief FIFF / MNE annotations: time-tagged textual marks (BAD_*, EDGE, custom) with onset, duration and description.
 *
 * Annotations are time-localized comments attached to a continuous
 * recording: rejected segments (``BAD_*''), recording edges (``EDGE''),
 * and arbitrary user-supplied marks. MNE-Python stores them as the
 * @c mne.Annotations type and writes them under
 * @c FIFFB_MNE_ANNOTATIONS; this header is the C++ mirror.
 *
 * @ref FiffAnnotation is one entry (onset in seconds since the
 * recording start, duration, description string), @ref FiffAnnotations is
 * the container; together they expose JSON and CSV serialization so the
 * same annotations can be exchanged with external tools that do not
 * speak FIFF (eg. BIDS sidecar files).
 */

#ifndef FIFF_ANNOTATIONS_H
#define FIFF_ANNOTATIONS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief One FIFF / MNE annotation: onset (s), duration (s) and description string.
 *
 * Field-for-field counterpart of one row of @c mne.Annotations in
 * MNE-Python. Onset is given in seconds relative to the recording start
 * (matching the @c first_samp / @c sfreq convention of @ref FiffRawData).
 */
struct FIFFSHARED_EXPORT FiffAnnotation
{
    double      onset;          /**< Onset in seconds. */
    double      duration;       /**< Duration in seconds (0 for point events). */
    QString     description;    /**< Label: "BAD", "BAD_ACQ_SKIP", etc. */
    QStringList channelNames;   /**< Empty = global annotation. */
    QString     comment;        /**< Optional free-form comment. */
    QVariantMap extras;         /**< Format-specific metadata. */
};

//=============================================================================================================
/**
 * @brief Container for @ref FiffAnnotation entries with FIFF, JSON and CSV serializers.
 *
 * Read and written under @c FIFFB_MNE_ANNOTATIONS so the annotation list
 * travels with the FIFF file. The JSON and CSV exporters let the same
 * annotations be consumed by BIDS sidecars and by external scoring
 * tools that do not understand FIFF directly.
 */
class FIFFSHARED_EXPORT FiffAnnotations
{
public:
    typedef QSharedPointer<FiffAnnotations> SPtr;            /**< Shared pointer type. */
    typedef QSharedPointer<const FiffAnnotations> ConstSPtr; /**< Const shared pointer type. */

    //=========================================================================================================
    /**
     * Constructs an empty FiffAnnotations object.
     */
    FiffAnnotations();

    //=========================================================================================================
    /**
     * Returns the number of annotations.
     *
     * @return Number of annotations.
     */
    int size() const;

    //=========================================================================================================
    /**
     * Returns true if no annotations are stored.
     *
     * @return True if empty.
     */
    bool isEmpty() const;

    //=========================================================================================================
    /**
     * Const subscript operator.
     *
     * @param[in] index    Index of the annotation.
     * @return Const reference to the annotation.
     */
    const FiffAnnotation& operator[](int index) const;

    //=========================================================================================================
    /**
     * Subscript operator.
     *
     * @param[in] index    Index of the annotation.
     * @return Reference to the annotation.
     */
    FiffAnnotation& operator[](int index);

    //=========================================================================================================
    /**
     * Appends an annotation.
     *
     * @param[in] annotation   The annotation to append.
     */
    void append(const FiffAnnotation& annotation);

    //=========================================================================================================
    /**
     * Appends an annotation by fields.
     *
     * @param[in] onset          Onset in seconds.
     * @param[in] duration       Duration in seconds.
     * @param[in] description    Description label.
     * @param[in] channelNames   Channel names (empty for global).
     * @param[in] comment        Optional comment.
     */
    void append(double onset, double duration, const QString& description,
                const QStringList& channelNames = QStringList(),
                const QString& comment = QString());

    //=========================================================================================================
    /**
     * Removes the annotation at the given index.
     *
     * @param[in] index    Index to remove.
     */
    void remove(int index);

    //=========================================================================================================
    /**
     * Removes all annotations.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns a const reference to the underlying vector.
     *
     * @return Const reference to annotation vector.
     */
    const QVector<FiffAnnotation>& toVector() const;

    //=========================================================================================================
    /**
     * Converts onset of annotation at index to a sample number.
     *
     * @param[in] index        Annotation index.
     * @param[in] sfreq        Sampling frequency in Hz.
     * @param[in] firstSample  First sample offset.
     * @return Sample number.
     */
    int onsetToSample(int index, double sfreq, int firstSample = 0) const;

    //=========================================================================================================
    /**
     * Converts end (onset+duration) of annotation at index to a sample number.
     *
     * @param[in] index        Annotation index.
     * @param[in] sfreq        Sampling frequency in Hz.
     * @param[in] firstSample  First sample offset.
     * @return Sample number.
     */
    int endToSample(int index, double sfreq, int firstSample = 0) const;

    //=========================================================================================================
    /**
     * Selects annotations whose description starts with the filter string.
     *
     * @param[in] descriptionFilter    Prefix to match.
     * @return Filtered annotations.
     */
    FiffAnnotations select(const QString& descriptionFilter) const;

    //=========================================================================================================
    /**
     * Selects annotations that reference the given channel name.
     *
     * @param[in] channelName    Channel name to match.
     * @return Filtered annotations.
     */
    FiffAnnotations selectByChannel(const QString& channelName) const;

    //=========================================================================================================
    /**
     * Returns annotations overlapping [tmin, tmax], clipping partially overlapping ones.
     *
     * @param[in] tmin    Start time in seconds.
     * @param[in] tmax    End time in seconds.
     * @return Cropped annotations.
     */
    FiffAnnotations crop(double tmin, double tmax) const;

    //=========================================================================================================
    /**
     * Reads annotations from a JSON file.
     *
     * @param[in] path    File path.
     * @return Parsed annotations.
     */
    static FiffAnnotations readJson(const QString& path);

    //=========================================================================================================
    /**
     * Writes annotations to a JSON file.
     *
     * @param[in] path     File path.
     * @param[in] annot    Annotations to write.
     * @return True on success.
     */
    static bool writeJson(const QString& path, const FiffAnnotations& annot);

    //=========================================================================================================
    /**
     * Reads annotations from a CSV file.
     *
     * @param[in] path    File path.
     * @return Parsed annotations.
     */
    static FiffAnnotations readCsv(const QString& path);

    //=========================================================================================================
    /**
     * Writes annotations to a CSV file.
     *
     * @param[in] path     File path.
     * @param[in] annot    Annotations to write.
     * @return True on success.
     */
    static bool writeCsv(const QString& path, const FiffAnnotations& annot);

    //=========================================================================================================
    /**
     * Auto-detects format and reads annotations.
     *
     * @param[in] path    File path (.json or .csv).
     * @return Parsed annotations.
     */
    static FiffAnnotations read(const QString& path);

    //=========================================================================================================
    /**
     * Auto-detects format and writes annotations.
     *
     * @param[in] path     File path (.json or .csv).
     * @param[in] annot    Annotations to write.
     * @return True on success.
     */
    static bool write(const QString& path, const FiffAnnotations& annot);

private:
    QVector<FiffAnnotation> m_annotations;  /**< Annotation storage. */
};

} // namespace FIFFLIB

#endif // FIFF_ANNOTATIONS_H
