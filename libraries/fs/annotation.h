//=============================================================================================================
/**
 * @file     annotation.h
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
 * @brief    Annotation class declaration
 *
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "colortable.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QSharedPointer>

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

class Label;
class Surface;

//=============================================================================================================
/**
 * Free surfer annotation contains vertix label relations and a color/name lookup table
 *
 * @brief Free surfer annotation
 */
class FSSHARED_EXPORT Annotation
{

public:
    typedef QSharedPointer<Annotation> SPtr;            /**< Shared pointer type for Annotation. */
    typedef QSharedPointer<const Annotation> ConstSPtr; /**< Const shared pointer type for Annotation. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    Annotation();

    //=========================================================================================================
    /**
     * Construts the annotation by reading it of the given file.
     *
     * @param[in] p_sFileName    Annotation file.
     */
    explicit Annotation(const QString& p_sFileName);

    //=========================================================================================================
    /**
     * Construts the annotation by reading it of the given file.
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     * @param[in] subjects_dir       Subjects directory.
     */
    explicit Annotation(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir);

    //=========================================================================================================
    /**
     * Construts the annotation by reading it of the given file.
     *
     * @param[in] path               path to surface directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     *
     * @return true if read sucessful, false otherwise.
     */
    explicit Annotation(const QString &path, qint32 hemi, const QString &surf);

    //=========================================================================================================
    /**
     * Destroys the annotation.
     */
    ~Annotation();

    //=========================================================================================================
    /**
     * Initializes the Annotation.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns whether Annotation is empty.
     *
     * @return true if is empty, false otherwise.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Returns the hemisphere id (0 = lh; 1 = rh)
     *
     * @return hemisphere id.
     */
    inline qint32 hemi() const;

    //=========================================================================================================
    /**
     * Returns the vertix indeces
     *
     * @return vertix indeces.
     */
    inline Eigen::VectorXi& getVertices();

    //=========================================================================================================
    /**
     * Returns the vertix indeces
     *
     * @return vertix indeces.
     */
    inline const Eigen::VectorXi getVertices() const;

    //=========================================================================================================
    /**
     * Returns the vertix labels
     *
     * @return vertix labels.
     */
    inline Eigen::VectorXi& getLabelIds();

    //=========================================================================================================
    /**
     * Returns the vertix labels
     *
     * @return vertix labels.
     */
    inline const Eigen::VectorXi getLabelIds() const;

    //=========================================================================================================
    /**
     * Returns the coloratable containing the label based nomenclature
     *
     * @return colortable.
     */
    inline Colortable& getColortable();

    //=========================================================================================================
    /**
     * Returns the coloratable containing the label based nomenclature
     *
     * @return colortable.
     */
    inline const Colortable getColortable() const;

    //=========================================================================================================
    /**
     * Reads a FreeSurfer annotation file
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     * @param[in] subjects_dir       Subjects directory.
     * @param[out] p_Annotation      The read annotation.
     *
     * @return true if read sucessful, false otherwise.
     */
    static bool read(const QString &subject_id, qint32 hemi, const QString &atlas, const QString &subjects_dir, Annotation &p_Annotation);

    //=========================================================================================================
    /**
     * Reads a FreeSurfer annotation file
     *
     * @param[in] path               path to label directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     * @param[out] p_Annotation      The read annotation.
     *
     * @return true if read sucessful, false otherwise.
     */
    static bool read(const QString &path, qint32 hemi, const QString &atlas, Annotation &p_Annotation);

    //=========================================================================================================
    /**
     * Reads an annotation of a file
     *
     * @param[in] p_sFileName    Annotation file.
     * @param[out] p_Annotation  the read annotation.
     *
     * @return true if successful, false otherwise.
     */
    static bool read(const QString &p_sFileName, Annotation &p_Annotation);

    //=========================================================================================================
    /**
     * python labels_from_parc
     *
     * Converts annotation to a label list and colortable
     *
     * @param[in] p_surf                 the surface to read the vertex positions from.
     * @param[out] p_qListLabels         the converted labels are appended to a given list. Stored data are not affected.
     * @param[out] p_qListLabelRGBAs     the converted label RGBAs are appended to a given list. Stored data are not affected.
     * @param[out] lLabelPicks           the label names which should be picked.
     *
     * @return true if successful, false otherwise.
     */
    bool toLabels(const Surface &p_surf,
                  QList<Label> &p_qListLabels,
                  QList<Eigen::RowVector4i> &p_qListLabelRGBAs,
                  const QStringList& lLabelPicks = QStringList()) const;

    //=========================================================================================================
    /**
     * annotation file path
     *
     * @return the surf file path.
     */
    inline QString filePath() const;

    //=========================================================================================================
    /**
     * annotation file name
     *
     * @return the surf file name.
     */
    inline QString fileName() const;

private:
    QString m_sFileName;        /**< Annotation file name. */
    QString m_sFilePath;        /**< Annotation file path. */

    qint32 m_iHemi;             /**< Hemisphere (lh = 0; rh = 1). */
    Eigen::VectorXi m_Vertices;        /**< Vertice indeces. */
    Eigen::VectorXi m_LabelIds;        /**< Vertice label ids. */

    Colortable m_Colortable;    /**< Lookup table label colors & ids. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 Annotation::hemi() const
{
    return m_iHemi;
}

//=============================================================================================================

inline bool Annotation::isEmpty() const
{
    return m_iHemi == -1;
}

//=============================================================================================================

inline Eigen::VectorXi& Annotation::getVertices()
{
    return m_Vertices;
}

//=============================================================================================================

inline const Eigen::VectorXi Annotation::getVertices() const
{
    return m_Vertices;
}

//=============================================================================================================

inline Eigen::VectorXi& Annotation::getLabelIds()
{
    return m_LabelIds;
}

//=============================================================================================================

inline const Eigen::VectorXi Annotation::getLabelIds() const
{
    return m_LabelIds;
}

//=============================================================================================================

inline Colortable& Annotation::getColortable()
{
    return m_Colortable;
}

//=============================================================================================================

inline const Colortable Annotation::getColortable() const
{
    return m_Colortable;
}

//=============================================================================================================

inline QString Annotation::filePath() const
{
    return m_sFilePath;
}

//=============================================================================================================

inline QString Annotation::fileName() const
{
    return m_sFileName;
}
} // NAMESPACE

#endif // ANNOTATION_H
