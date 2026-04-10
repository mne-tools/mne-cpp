//=============================================================================================================
/**
 * @file     fiff_annotations.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    FiffAnnotations class declaration.
 *
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
 * @brief Single annotation entry.
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
 * @brief Container for FIFF annotations with I/O in JSON and CSV formats.
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
