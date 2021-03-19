//=============================================================================================================
/**
 * @file     annotationset.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    AnnotationSet class declaration
 *
 */

#ifndef ANNOTATION_SET_H
#define ANNOTATION_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "annotation.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QSharedPointer>
#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class SurfaceSet;

//=============================================================================================================
/**
 * Annotation set
 *
 * @brief Annotation set
 */
class FSSHARED_EXPORT AnnotationSet
{
public:
    typedef QSharedPointer<AnnotationSet> SPtr;            /**< Shared pointer type for AnnotationSet. */
    typedef QSharedPointer<const AnnotationSet> ConstSPtr; /**< Const shared pointer type for AnnotationSet. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    AnnotationSet();

    //=========================================================================================================
    /**
     * Construts the surface set by reading it of the given files.
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     * @param[in] subjects_dir       Subjects directory.
     */
    explicit AnnotationSet(const QString &subject_id, qint32 hemi, const QString &atlas, const QString &subjects_dir);

    //=========================================================================================================
    /**
     * Construts the surface set by reading it of the given files.
     *
     * @param[in] path               path to surface directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     */
    explicit AnnotationSet(const QString &path, qint32 hemi, const QString &atlas);

    //=========================================================================================================
    /**
     * Constructs an annotation set by assembling given annotations
     *
     * @param[in] p_LHAnnotation    Left hemisphere annotation.
     * @param[in] p_RHAnnotation    Right hemisphere annotation.
     */
    explicit AnnotationSet(const Annotation& p_LHAnnotation, const Annotation& p_RHAnnotation);

    //=========================================================================================================
    /**
     * Constructs an annotation set by reading from annotation files
     *
     * @param[in] p_sLHFileName  Left hemisphere annotation file.
     * @param[in] p_sRHFileName  Right hemisphere annotation file.
     */
    explicit AnnotationSet(const QString& p_sLHFileName, const QString& p_sRHFileName);

    //=========================================================================================================
    /**
     * Destroys the annotation set.
     */
    ~AnnotationSet(){}

    //=========================================================================================================
    /**
     * Initializes the AnnotationSet.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns The Annotation set map
     *
     * @return the annotation set map.
     */
    inline QMap<qint32, Annotation>& data();

    //=========================================================================================================
    /**
     * True if AnnotationSet is empty.
     *
     * @return true if AnnotationSet is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Insert an annotation
     *
     * @param[in] p_Annotation  Annotation to insert.
     */
    void insert(const Annotation& p_Annotation);

    //=========================================================================================================
    /**
     * Reads different annotation files and assembles them to a AnnotationSet
     *
     * @param[in] p_sLHFileName  Left hemisphere annotation file.
     * @param[in] p_sRHFileName  Right hemisphere annotation file.
     * @param[out] p_AnnotationSet   The read annotation set.
     *
     * @return true if succesfull, false otherwise.
     */
    static bool read(const QString& p_sLHFileName, const QString& p_sRHFileName, AnnotationSet &p_AnnotationSet);

    //=========================================================================================================
    /**
     * python labels_from_parc
     *
     * Converts annotation to a label list and colortable
     *
     * @param[in] p_surfSet              the SurfaceSet to read the vertex positions from.
     * @param[out] p_qListLabels         the converted labels are appended to a given list. Stored data are not affected.
     * @param[out] p_qListLabelRGBAs     the converted label RGBAs are appended to a given list. Stored data are not affected.
     * @param[out] lLabelPicks           the label names which should be picked.
     *
     * @return true if successful, false otherwise.
     */
    bool toLabels(const SurfaceSet &p_surfSet,
                  QList<Label> &p_qListLabels,
                  QList<Eigen::RowVector4i> &p_qListLabelRGBAs,
                  const QStringList& lLabelPicks = QStringList()) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access annotation by index
     *
     * @param[in] idx    the hemisphere index (0 or 1).
     *
     * @return Annotation related to the parameter index.
     */
    Annotation& operator[] (qint32 idx);

    //=========================================================================================================
    /**
     * Subscript operator [] to access annotation by index
     *
     * @param[in] idx    the hemisphere index (0 or 1).
     *
     * @return Annotation related to the parameter index.
     */
    const Annotation operator[] (qint32 idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access annotation by identifier
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return Annotation related to the parameter identifier.
     */
    Annotation& operator[] (QString idt);

    //=========================================================================================================
    /**
     * Subscript operator [] to access annotation by identifier
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return Annotation related to the parameter identifier.
     */
    const Annotation operator[] (QString idt) const;

    //=========================================================================================================
    /**
     * Returns number of loaded hemispheres
     *
     * @return number of loaded hemispheres.
     */
    inline qint32 size() const;

private:
    QMap<qint32, Annotation> m_qMapAnnots;   /**< Hemisphere annotations (lh = 0; rh = 1). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QMap<qint32, Annotation>& AnnotationSet::data()
{
    return m_qMapAnnots;
}

//=============================================================================================================

inline bool AnnotationSet::isEmpty() const
{
    return m_qMapAnnots.isEmpty();
}

//=============================================================================================================

inline qint32 AnnotationSet::size() const
{
    return m_qMapAnnots.size();
}
} // NAMESPACE

#endif // ANNOTATION_SET_H
