//=============================================================================================================
/**
 * @file     mne_sourcespace.h
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
 * @brief    MNESourceSpace class declaration.
 *
 */

#ifndef MNE_SOURCESPACE_H
#define MNE_SOURCESPACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_hemisphere.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff.h>

#include <algorithm>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB
{
    class Label;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Source Space descritpion
 *
 * @brief Source Space descritpion
 */
class MNESHARED_EXPORT MNESourceSpace
{
public:
    typedef QSharedPointer<MNESourceSpace> SPtr;            /**< Shared pointer type for MNESourceSpace. */
    typedef QSharedPointer<const MNESourceSpace> ConstSPtr; /**< Const shared pointer type for MNESourceSpace. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    MNESourceSpace();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MNESourceSpace   MNE forward solution.
     */
    MNESourceSpace(const MNESourceSpace &p_MNESourceSpace);

    //=========================================================================================================
    /**
     * Destroys the MNE forward solution
     */
    ~MNESourceSpace();

    //=========================================================================================================
    /**
     * Initializes MNE source space.
     */
    void clear();

    //=========================================================================================================
    /**
     * True if MNE Source Space is empty.
     *
     * @return true if MNE Source Space is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_find_source_space_hemi function
     *
     * Returns the hemisphere id ( FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI) for a source space.
     *
     * @param[in] p_Hemisphere the hemisphere to investigate.
     *
     * @return the deduced hemisphere id.
     */
    static qint32 find_source_space_hemi(MNEHemisphere& p_Hemisphere);

    //=========================================================================================================
    /**
     * Returns the Zero based (different to MATLAB) indices of the used vertices of both hemispheres
     *
     * @return the hemisphere vertices.
     */
    QList<Eigen::VectorXi> get_vertno() const;

    //=========================================================================================================
    /**
     * Find vertex numbers and indices from label
     *
     * @param[in] label      Source space label.
     * @param[out] src_sel   array of int (idx.size() = vertno[0].size() + vertno[1].size()).
     *                       Indices of the selected vertices in sourse space
     *
     * @return vertno list of length 2 Vertex numbers for lh and rh.
     */
    QList<Eigen::VectorXi> label_src_vertno_sel(const FSLIB::Label &p_label, Eigen::VectorXi &src_sel) const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_patch_info function
     *
     * Generate the patch information from the 'nearest' vector in a source space. For vertex in the source
     * space it provides the list of neighboring vertices in the high resolution triangulation.
     *
     * @param[in, out] p_Hemisphere  The source space.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool patch_info(MNEHemisphere &p_Hemisphere);//VectorXi& nearest, QList<VectorXi>& pinfo);@param[in] nearest   The nearest vector of the source space.@param[in, out] pinfo    The requested patch information.

    //=========================================================================================================
    /**
     * Reduces a source space to selected regions
     *
     * @param[in] p_qListLabels  ROIs.
     *
     * @return the reduced source space.
     */
    MNESourceSpace pick_regions(const QList<FSLIB::Label> &p_qListLabels) const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_read_source_spaces function
     *
     * Reads source spaces from a fif file
     *
     * @param[in, out] p_pStream         The opened fif file.
     * @param[in] add_geom          Add geometry information to the source spaces.
     * @param[in, out] p_SourceSpace    The read source spaces.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool readFromStream(FIFFLIB::FiffStream::SPtr& p_pStream,
                               bool add_geom,
                               MNESourceSpace& p_SourceSpace);

    //=========================================================================================================
    /**
     * Returns the number of stored hemispheres 0, 1 or 2
     *
     * @return number of stored hemispheres.
     */
    inline qint32 size() const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_transform_source_space_to function
     * Wrapper for the MNESourceSpace transform_source_space_to member function
     *
     * Note: In difference to mne-matlab this is not a static function. This is a method of the MNESourceSpace
     *       class, that's why a tree object doesn't need to be handed to the function.
     *
     * Transforms source space data to the desired coordinate system
     *
     * @param[in] dest destination check code.
     * @param[in] trans transformation information.
     *
     * @return true if succeeded, false otherwise.
     */
    bool transform_source_space_to(FIFFLIB::fiff_int_t dest,
                                   FIFFLIB::FiffCoordTrans& trans);

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the write_source_spaces_to_fid function
     *
     * Write the source spaces to a FIF stream
     *
     * @param[in] p_pStream  The stream to write to.
     */
    void writeToStream(FIFFLIB::FiffStream* p_pStream);

    //=========================================================================================================
    /**
     * Subscript operator [] to access parameter values by index
     *
     * @param[in] idx    the hemisphere index (0 or 1).
     *
     * @return Hemisphere related to the parameter index.
     */
    MNEHemisphere& operator[] (qint32 idx);

    //=========================================================================================================
    /**
     * Subscript operator [] to access parameter values by index
     *
     * @param[in] idx    the hemisphere index (0 or 1).
     *
     * @return Hemisphere related to the parameter index.
     */
    const MNEHemisphere& operator[] (qint32 idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access parameter values by index
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return Hemisphere related to the parameter identifier.
     */
    MNEHemisphere& operator[] (QString idt);

    //=========================================================================================================
    /**
     * Subscript operator [] to access parameter values by index
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return Hemisphere related to the parameter identifier.
     */
    const MNEHemisphere& operator[] (QString idt) const;

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const MNESourceSpace &a, const MNESourceSpace &b);

private:

    //=========================================================================================================
    /**
     * Definition of the complete_source_space_info function in e.g. mne_read_source_spaces.m, mne_read_bem_surfaces.m
     *
     * Completes triangulation info
     *
     * @param[in, out] p_pHemisphere   Hemisphere to be completed.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool complete_source_space_info(MNEHemisphere& p_Hemisphere);

    //=========================================================================================================
    /**
     * Definition of the read_source_space function in e.g. mne_read_source_spaces.m, mne_read_bem_surfaces.m
     *
     * Reads a single source space (hemisphere)
     *
     * @param[in] p_pStream         The opened fif file.
     * @param[in] p_Tree            Search for the source space here.
     * @param[in, out] p_pHemisphere    The read source space (hemisphere).
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_source_space(FIFFLIB::FiffStream::SPtr& p_pStream,
                                  const FIFFLIB::FiffDirNode::SPtr& p_Tree,
                                  MNEHemisphere& p_Hemisphere);

private:
    QList<MNEHemisphere> m_qListHemispheres;    /**< List of the hemispheres containing the source space information. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNESourceSpace::isEmpty() const
{
    return m_qListHemispheres.size() == 0;
}

//=============================================================================================================

inline qint32 MNESourceSpace::size() const
{
    return m_qListHemispheres.size();
}

//=============================================================================================================

inline bool operator== (const MNESourceSpace &a, const MNESourceSpace &b)
{
    return (a.m_qListHemispheres == b.m_qListHemispheres);
}
} // NAMESPACE

#endif // MNE_SOURCESPACE_H
