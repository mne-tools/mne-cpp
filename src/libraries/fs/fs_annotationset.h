//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_annotationset.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Bi-hemispheric pair of FreeSurfer annotations (lh + rh) for one parcellation atlas.
 *
 * Whole-brain parcellation analyses load one annotation file per
 * hemisphere — typically @c lh.aparc.annot and @c rh.aparc.annot — that
 * share an atlas-specific colortable but assign labels independently per
 * hemisphere. @ref FsAnnotationSet bundles both into a single container so
 * downstream code can iterate over hemispheres uniformly and pair the
 * result with a matching @ref FsSurfaceSet of the same subject.
 *
 * The class accepts the standard FreeSurfer atlas names exposed by
 * @c recon-all (@c aparc, @c aparc.a2009s, @c aparc.DKTatlas40,
 * @c BA, @c BA.thresh, custom atlases produced by
 * @c mris_ca_label…) and resolves them against
 * @c $SUBJECTS_DIR/<id>/label/{lh|rh}.<atlas>.annot, mirroring the path
 * convention used by @c mne.read_labels_from_annot in MNE-Python.
 */

#ifndef FS_ANNOTATIONSET_H
#define FS_ANNOTATIONSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "fs_annotation.h"

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

class FsSurfaceSet;

//=============================================================================================================
/**
 * @brief Container holding the lh and/or rh @ref FsAnnotation for one parcellation atlas.
 *
 * Keyed on hemisphere id (0 = lh, 1 = rh). When loaded from a single atlas
 * name both hemispheres share the same colortable layout, so cross-hemisphere
 * region lookup is consistent. The set is intended to be aligned
 * one-to-one with an @ref FsSurfaceSet sharing the same subject and vertex
 * count per hemisphere.
 */
class FSSHARED_EXPORT FsAnnotationSet
{
public:
    typedef QSharedPointer<FsAnnotationSet> SPtr;            /**< Shared pointer type for FsAnnotationSet. */
    typedef QSharedPointer<const FsAnnotationSet> ConstSPtr; /**< Const shared pointer type for FsAnnotationSet. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    FsAnnotationSet();

    //=========================================================================================================
    /**
     * Construts the surface set by reading it of the given files.
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     * @param[in] subjects_dir       Subjects directory.
     */
    explicit FsAnnotationSet(const QString &subject_id, qint32 hemi, const QString &atlas, const QString &subjects_dir);

    //=========================================================================================================
    /**
     * Construts the surface set by reading it of the given files.
     *
     * @param[in] path               path to surface directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     */
    explicit FsAnnotationSet(const QString &path, qint32 hemi, const QString &atlas);

    //=========================================================================================================
    /**
     * Constructs an annotation set by assembling given annotations
     *
     * @param[in] p_LHAnnotation    Left hemisphere annotation.
     * @param[in] p_RHAnnotation    Right hemisphere annotation.
     */
    explicit FsAnnotationSet(const FsAnnotation& p_LHAnnotation, const FsAnnotation& p_RHAnnotation);

    //=========================================================================================================
    /**
     * Constructs an annotation set by reading from annotation files
     *
     * @param[in] p_sLHFileName  Left hemisphere annotation file.
     * @param[in] p_sRHFileName  Right hemisphere annotation file.
     */
    explicit FsAnnotationSet(const QString& p_sLHFileName, const QString& p_sRHFileName);

    //=========================================================================================================
    /**
     * Destroys the annotation set.
     */
    ~FsAnnotationSet(){}

    //=========================================================================================================
    /**
     * Initializes the FsAnnotationSet.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns The FsAnnotation set map
     *
     * @return the annotation set map.
     */
    inline QMap<qint32, FsAnnotation>& data();

    //=========================================================================================================
    /**
     * True if FsAnnotationSet is empty.
     *
     * @return true if FsAnnotationSet is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Insert an annotation
     *
     * @param[in] p_Annotation  FsAnnotation to insert.
     */
    void insert(const FsAnnotation& p_Annotation);

    //=========================================================================================================
    /**
     * Reads different annotation files and assembles them to a FsAnnotationSet
     *
     * @param[in] p_sLHFileName  Left hemisphere annotation file.
     * @param[in] p_sRHFileName  Right hemisphere annotation file.
     * @param[out] p_AnnotationSet   The read annotation set.
     *
     * @return true if succesfull, false otherwise.
     */
    static bool read(const QString& p_sLHFileName, const QString& p_sRHFileName, FsAnnotationSet &p_AnnotationSet);

    //=========================================================================================================
    /**
     * python labels_from_parc
     *
     * Converts annotation to a label list and colortable
     *
     * @param[in] p_surfSet              the FsSurfaceSet to read the vertex positions from.
     * @param[out] p_qListLabels         the converted labels are appended to a given list. Stored data are not affected.
     * @param[out] p_qListLabelRGBAs     the converted label RGBAs are appended to a given list. Stored data are not affected.
     * @param[out] lLabelPicks           the label names which should be picked.
     *
     * @return true if successful, false otherwise.
     */
    bool toLabels(const FsSurfaceSet &p_surfSet,
                  QList<FsLabel> &p_qListLabels,
                  QList<Eigen::RowVector4i> &p_qListLabelRGBAs,
                  const QStringList& lLabelPicks = QStringList()) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access annotation by index
     *
     * @param[in] idx    the hemisphere index (0 or 1).
     *
     * @return FsAnnotation related to the parameter index.
     */
    FsAnnotation& operator[] (qint32 idx);

    //=========================================================================================================
    /**
     * Subscript operator [] to access annotation by index
     *
     * @param[in] idx    the hemisphere index (0 or 1).
     *
     * @return FsAnnotation related to the parameter index.
     */
    const FsAnnotation operator[] (qint32 idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access annotation by identifier
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return FsAnnotation related to the parameter identifier.
     */
    FsAnnotation& operator[] (QString idt);

    //=========================================================================================================
    /**
     * Subscript operator [] to access annotation by identifier
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return FsAnnotation related to the parameter identifier.
     */
    const FsAnnotation operator[] (QString idt) const;

    //=========================================================================================================
    /**
     * Returns number of loaded hemispheres
     *
     * @return number of loaded hemispheres.
     */
    inline qint32 size() const;

private:
    QMap<qint32, FsAnnotation> m_qMapAnnots;   /**< Hemisphere annotations (lh = 0; rh = 1). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QMap<qint32, FsAnnotation>& FsAnnotationSet::data()
{
    return m_qMapAnnots;
}

//=============================================================================================================

inline bool FsAnnotationSet::isEmpty() const
{
    return m_qMapAnnots.isEmpty();
}

//=============================================================================================================

inline qint32 FsAnnotationSet::size() const
{
    return m_qMapAnnots.size();
}
} // NAMESPACE

#endif // FS_ANNOTATIONSET_H
