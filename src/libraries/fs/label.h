//=============================================================================================================
/**
 * @file     label.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Label class declaration
 *
 */

#ifndef LABEL_H
#define LABEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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

const static Eigen::MatrixX3i defaultTris(0,3);

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Surface;

//=============================================================================================================
/**
 * A Freesurfer/MNE label with vertices restricted to one hemisphere
 *
 * @brief Freesurfer/MNE label
 */
class FSSHARED_EXPORT Label
{
public:
    typedef QSharedPointer<Label> SPtr;            /**< Shared pointer type for Label class. */
    typedef QSharedPointer<const Label> ConstSPtr; /**< Const shared pointer type for Label class. */
    
    //=========================================================================================================
    /**
     * Default constructor
     */
    Label();

    //=========================================================================================================
    /**
     * Constructs a label
     *
     * @param[in] p_vertices     Vertices.
     * @param[in] p_pos          Positions.
     * @param[in] p_values       Values.
     * @param[in] p_hemi         Hemisphere (lh = 0; rh = 1).
     * @param[in] p_name         label names.
     * @param[in] p_id           label id (optional, default = -1).
     */
    Label(const Eigen::VectorXi &p_vertices,
          const Eigen::MatrixX3f &p_pos,
          const Eigen::VectorXd &p_values,
          qint32 p_hemi, const QString &p_name,
          qint32 p_id = -1);
    
    //=========================================================================================================
    /**
     * Destroys the Label class.
     */
    ~Label();

    //=========================================================================================================
    /**
     * Initializes the Label.
     */
    void clear();

    //=========================================================================================================
    /**
     * True if Label is empty.
     *
     * @return true if Label is empty, false otherwise.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Select tris for this label from a given surface file.
     *
     * @param[in] p_Surface      to generate the label tris from.
     *
     * @return the generated tris.
     */
    Eigen::MatrixX3i selectTris(const Surface & p_Surface);

    //=========================================================================================================
    /**
     * Select tris for this label from a given tri matrix.
     *
     * @param[in] p_matTris      tris from which the selection should be made.
     *
     * @return the generated tris.
     */
    Eigen::MatrixX3i selectTris(const Eigen::MatrixX3i &p_matTris);

    //=========================================================================================================
    /**
     * mne_read_label_file
     *
     * Reads a Label from a FreeSurfer label file.
     * This is based on the FreeSurfer read_label routine
     * SUBJECTS_DIR environment variable is not consulted for the standard location
     *
     * @param[in] p_sFileName    label file name.
     * @param[in] p_Label        read label.
     *
     * @return true if successful, false otherwise.
     */
    static bool read(const QString& p_sFileName, Label &p_Label);

public:
    QString comment;            /**< Comment from the first line of the label file. */
    Eigen::VectorXi vertices;   /**< Vertex indices (0 based). */
    Eigen::MatrixX3f pos;       /**< Locations in meters. */
    Eigen::VectorXd values;     /**< Values at the vertices. */
    qint32 hemi;                /**< Hemisphere (lh = 0; rh = 1). */
//    qint32 hemi;                        /**< Hemisphere (lh = 0; rh = 1; both = 2). */ Don't mix both hemis - KISS principle
    QString name;               /**< Name of the label. */
    qint32 label_id;            /**< Label id (optional). */
//    Eigen::MatrixX3i tris;     /**< Tris for plotting (optional). */

//    QMap<qint32, VectorXi> vertices;    /**< Vertex indices (0 based). */
//    QMap<qint32, Eigen::MatrixX3d> pos;        /**< Locations in meters. */
//    QMap<qint32, VectorXd> values;      /**< Values at the vertices. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool Label::isEmpty() const
{
    return this->hemi == -1;
}
} // NAMESPACE

#ifndef metatype_label
#define metatype_label
Q_DECLARE_METATYPE(FSLIB::Label);
#endif

#endif // LABEL_H
