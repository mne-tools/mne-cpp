//=============================================================================================================
/**
* @file     mne.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the MNE class declaration, which provides static wrapper functions to stay consistent with
*           mne matlab toolbox
*
*/

#ifndef MNE_H
#define MNE_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include "include/mne_forwardsolution.h"
#include "include/mne_hemisphere.h"
#include "include/mne_sourcespace.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../fiff/include/fiff_constants.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//using namespace SOURCELAB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE MNE WRAPPER CLASS
* @brief The MNE class provides wrapper functions to stay consistent with mne matlab toolbox.
*/

class MNESHARED_EXPORT MNE
{

public:

    //=========================================================================================================
    /**
    * dtor
    */
    virtual ~MNE()
    { }


    //=========================================================================================================
    /**
    * mne_block_diag - decoding part
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNEForwardSolution::extract_block_diag static function
    */
    //    static inline MatrixXf extract_block_diag(MatrixXf& A, qint32 n);





    //ToDo make this part of fiff Info
    static qint32 get_current_comp(FiffInfo* info)
    {
        qint32 comp = 0;
        qint32 first_comp = -1;

        qint32 k = 0;
        for (k = 0; k < info->nchan; ++k)
        {
            if (info->chs[k].kind == FIFFV_MEG_CH)
            {
                comp = info->chs[k].coil_type >> 16;
                if (first_comp < 0)
                    first_comp = comp;
                else if (comp != first_comp)
                    printf("Compensation is not set equally on all MEG channels");
            }
        }
        return comp;
    }







    //=========================================================================================================
    /**
    * mne_block_diag - encoding part
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNEForwardSolution::make_block_diag static function
    *
    * Make a sparse block diagonal matrix
    *
    * Returns a sparse block diagonal, diagonalized from the elements in "A". "A" is ma x na, comprising
    * bdn=(na/"n") blocks of submatrices. Each submatrix is ma x "n", and these submatrices are placed down
    * the diagonal of the matrix.
    *
    * @param[in, out] A Matrix which should be diagonlized
    * @param[in, out] n Columns of the submatrices
    *
    * @return A sparse block diagonal, diagonalized from the elements in "A".
    */
    static inline SparseMatrix<float> make_block_diag(MatrixXf& A, qint32 n)
    {
        return MNEForwardSolution::make_block_diag(A, n);

    }



//    %
//    % [comp] = mne_make_compensator(info,from,to,exclude_comp_chs)
//    %
//    % info              - measurement info as returned by the fif reading routines
//    % from              - compensation in the input data
//    % to                - desired compensation in the output
//    % exclude_comp_chs  - exclude compensation channels from the output (optional)
//    %

//    %
//    % Create a compensation matrix to bring the data from one compensation
//    % state to another
//    %
//    function [comp] =
//    mne_make_compensator(info,from,to,exclude_comp_chs)


    //=========================================================================================================
    /**
    * ToDo make this part of FiffInfo
    * mne_make_compensator
    *
    * ### MNE toolbox root function ###
    *
    * Create a compensation matrix to bring the data from one compensation state to another
    *
    * @param[in] info               measurement info as returned by the fif reading routines
    * @param[in] from               compensation in the input data
    * @param[in] to                 desired compensation in the output
    * @param[out] comp              Compensation Matrix
    * @param[in] exclude_comp_chs   exclude compensation channels from the output (optional)
    *
    * @return true if succeeded, false otherwise
    */
    static bool make_compensator(FiffInfo* info, fiff_int_t from, fiff_int_t to, FiffCtfComp& ctf_comp, bool exclude_comp_chs = false)
    {
        qDebug() << "make_compensator not debugged jet";

        MatrixXf C1, C2, comp_tmp;

        qDebug() << "Todo add all need ctf variables.";
        if(ctf_comp.data)
            delete ctf_comp.data;
        ctf_comp.data = new FiffNamedMatrix();

        if (from == to)
        {
            ctf_comp.data->data = MatrixXf::Zero(info->nchan, info->nchan);
            return false;
        }

        if (from == 0)
            C1 = MatrixXf::Zero(info->nchan,info->nchan);
        else
        {
            if (!make_compensator(info,from, C1))
            {
                printf("Cannot create compensator C1\n");
                printf("Desired compensation matrix (kind = %d) not found\n",from);
                return false;
            }
        }

        if (to == 0)
            C2 = MatrixXf::Zero(info->nchan,info->nchan);
        else
        {
            if (!make_compensator(info, to, C2))
            {
                printf("Cannot create compensator C2\n");
                printf("Desired compensation matrix (kind = %d) not found\n",to);
                return false;
            }
        }
        //
        //   s_orig = s_from + C1*s_from = (I + C1)*s_from
        //   s_to   = s_orig - C2*s_orig = (I - C2)*s_orig
        //   s_to   = (I - C2)*(I + C1)*s_from = (I + C1 - C2 - C2*C1)*s_from
        //
        comp_tmp = MatrixXf::Identity(info->nchan,info->nchan) + C1 - C2 - C2*C1;

        qint32 k;
        if (exclude_comp_chs)
        {
            VectorXi pick  = MatrixXi::Zero(1,info->nchan);
            qint32 npick = 0;
            for (k = 0; k < info->nchan; ++k)
            {
                if (info->chs[k].kind != FIFFV_REF_MEG_CH)
                {
                    pick(npick) = k;
                    ++npick;
                }
            }
            if (npick == 0)
            {
                printf("Nothing remains after excluding the compensation channels\n");
                return false;
            }

            ctf_comp.data->data.resize(npick,info->nchan);
            for (k = 0; k < npick; ++k)
                ctf_comp.data->data.row(k) = comp_tmp.block(pick(k), 0, 1, info->nchan);
        }
        return true;
    }





    //=========================================================================================================




//    function this_comp = make_compensator(info,kind)


    static bool make_compensator(FiffInfo* info, fiff_int_t kind, MatrixXf& this_comp)
    {
        qDebug() << "make_compensator not debugged jet";

        FiffNamedMatrix* this_data;
        MatrixXf presel, postsel;
        qint32 k, col, c, ch, row, row_ch, channelAvailable;
        for (k = 0; k < info->comps.size(); ++k)
        {
            if (info->comps[k]->kind == kind)
            {
                this_data = info->comps[k]->data;
                //
                //   Create the preselector
                //
                presel  = MatrixXf::Zero(this_data->ncol,info->nchan);
                for(col = 0; col < this_data->ncol; ++col)
                {
                    channelAvailable = 0;
                    for (c = 0; c < info->ch_names.size(); ++c)
                    {
                        if (QString::compare(this_data->col_names.at(col),info->ch_names.at(c)) == 0)
                        {
                            ++channelAvailable;
                            ch = c;
                        }
                    }

                    if (channelAvailable == 0)
                    {
                        printf("Channel %s is not available in data\n",this_data->col_names.at(col).toUtf8().constData());
                        return false;
                    }
                    else if (channelAvailable > 1)
                    {
                        printf("Ambiguous channel %s",this_data->col_names.at(col).toUtf8().constData());
                        return false;
                    }
                    presel(col,ch) = 1.0;
                }
                //
                //   Create the postselector
                //
                postsel = MatrixXf::Zero(info->nchan,this_data->nrow);
                for (c = 0; c  < info->nchan; ++c)
                {

                    channelAvailable = 0;
                    for (row = 0; row < this_data->row_names.size(); ++row)
                    {
                        if (QString::compare(this_data->col_names.at(c),info->ch_names.at(row)) == 0)
                        {
                            ++channelAvailable;
                            row_ch = row;
                        }
                    }

                    if (channelAvailable > 1)
                    {
                        printf("Ambiguous channel %s", info->ch_names.at(c).toUtf8().constData());
                        return false;
                    }
                    else if (channelAvailable == 1)
                    {
                        postsel(c,row_ch) = 1.0;
                    }

                }
                this_comp = postsel*this_data->data*presel;
                return true;
            }
        }
        this_comp = defaultMatrixXf;
        return false;
    }



















    //=========================================================================================================

//Todo: Make this part of the raw.info
//    function new_chs = mne_set_current_comp(chs,value)
//    %
//    % mne_set_current_comp(chs,value)
//    %
//    % Set the current compensation value in the channel info structures
//    %


    static QList<FiffChInfo> set_current_comp(QList<FiffChInfo>& chs, fiff_int_t value)
    {
        QList<FiffChInfo> new_chs;
        qint32 k;
        fiff_int_t coil_type;

        for(k = 0; k < chs.size(); ++k)
        {
            new_chs.append(FiffChInfo(&chs[k]));
        }

        qint32 lower_half = 65535;// hex2dec('FFFF');
        for (k = 0; k < chs.size(); ++k)
        {
            if (chs[k].kind == FIFFV_MEG_CH)
            {
                coil_type = chs[k].coil_type & lower_half;
                new_chs[k].coil_type = (coil_type | (value << 16));
            }
        }
        return new_chs;
    }





    //=========================================================================================================
    /**
    * make_projector
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo::make_projector static function
    * There exists also a member function which should be preferred:
    * make_projector(MatrixXf& proj, MatrixXf& U = defaultUMatrix)
    *
    * Make an SSP operator
    *
    * @param[in] projs      A set of projection vectors
    * @param[in] ch_names   A cell array of channel names
    * @param[out] proj      The projection operator to apply to the data
    * @param[in] bads       Bad channels to exclude
    * @param[out] U         The orthogonal basis of the projection vectors (optional)
    *
    * @return nproj - How many items in the projector
    */
    inline static fiff_int_t make_projector(QList<FiffProj*>& projs, QStringList& ch_names, MatrixXf& proj, QStringList& bads = defaultQStringList, MatrixXf& U = defaultMatrixXf)
    {
        return FiffInfo::make_projector(projs, ch_names, proj, bads, U);
    }


    //=========================================================================================================
    /**
    * mne_make_projector_info
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo::make_projector_info(FiffInfo* info, MatrixXf& proj) static function
    * There exists also a member functions which should be preferred:
    * FiffInfo make_projector_info(MatrixXf& proj)
    *
    * Make a SSP operator using the meas info
    *
    * @param[in] info       Fiff measurement info
    * @param[out] proj      The projection operator to apply to the data
    *
    * @return nproj - How many items in the projector
    */
    static inline qint32 make_projector_info(FiffInfo* info, MatrixXf& proj)
    {
        return info->make_projector_info(proj);
    }


    //=========================================================================================================
    /**
    * mne_find_source_space_hemi
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace::find_source_space_hemi static function
    *
    * Returns the hemisphere id (FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI) for a source space.
    *
    * @param[in] p_pHemisphere the hemisphere to investigate
    *
    * @return the deduced hemisphere id
    */
    inline static qint32 find_source_space_hemi(MNEHemisphere* p_pHemisphere)
    {
        return MNESourceSpace::find_source_space_hemi(p_pHemisphere);
    }

    //=========================================================================================================
    /**
    * mne_patch_info
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace::patch_info static function
    *
    * @param [in] nearest   The nearest vector of the source space.
    * @param [out] pinfo    The requested patch information.
    *
    * @return true if succeeded, false otherwise
    */
    static bool patch_info(VectorXi& nearest, QList<VectorXi>& pinfo)
    {
        return MNESourceSpace::patch_info(nearest, pinfo);
    }

















//    function [eventlist] = mne_read_events(filename)
//    %
//    % [eventlist] = mne_read_events(filename)
//    %
//    % Read an event list from a fif file
//    %

    static bool read_events(QString& p_sFileName, MatrixXi& eventlist)
    {
        //
        // Open file
        //
        FiffFile* t_pFile = new FiffFile(p_sFileName);
        FiffDirTree* t_pTree = NULL;
        QList<FiffDirEntry>* t_pDir = NULL;

        if(!t_pFile->open(t_pTree, t_pDir))
        {
            if(t_pTree)
                delete t_pTree;

            if(t_pDir)
                delete t_pDir;

            return false;
        }

        //
        //   Find the desired block
        //
        QList<FiffDirTree*> events = t_pTree->dir_tree_find(FIFFB_MNE_EVENTS);

        if (events.size() == 0)
        {
            printf("Could not find event data\n");
            delete t_pFile;
            delete t_pTree;
            delete t_pDir;
            return false;
        }

        qint32 k, nelem;
        fiff_int_t kind, pos;
        FiffTag* t_pTag = NULL;
        quint32* serial_eventlist = NULL;
        for(k = 0; k < events[0]->nent; ++k)
        {
            kind = events[0]->dir[k].kind;
            pos  = events[0]->dir[k].pos;
            if (kind == FIFF_MNE_EVENT_LIST)
            {
                FiffTag::read_tag(t_pFile,t_pTag,pos);
                if(t_pTag->type == FIFFT_UINT)
                {
                    serial_eventlist = t_pTag->toUnsignedInt();
                    nelem = t_pTag->size/4;
                }
                break;
            }
        }

        if(serial_eventlist == NULL)
        {
            delete t_pFile;
            delete t_pTree;
            delete t_pDir;
            delete t_pTag;
            printf("Could not find any events\n");
            return false;
        }
        else
        {
            eventlist.resize(nelem/3,3);
            for(k = 0; k < nelem/3; ++k)
            {
                eventlist(k,0) = serial_eventlist[k*3];
                eventlist(k,1) = serial_eventlist[k*3+1];
                eventlist(k,2) = serial_eventlist[k*3+2];
            }
        }

        delete t_pFile;
        delete t_pTree;
        delete t_pDir;
        delete t_pTag;
        return true;
    }

















    //=========================================================================================================
    /**
    * mne_read_forward_solution
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNEForwardSolution::read_forward_solution static function
    *
    * Reads a forward solution from a fif file
    *
    * @param [in] p_sFile       The name of the file
    * @param [out] fwd A forward solution from a fif file
    * @param [in] force_fixed   Force fixed source orientation mode? (optional)
    * @param [in] surf_ori      Use surface based source coordinate system? (optional)
    * @param [in] include       Include these channels (optional)
    * @param [in] exclude       Exclude these channels (optional)
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool read_forward_solution(QString& p_sFile, MNEForwardSolution*& fwd, bool force_fixed = false, bool surf_ori = false, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList)
    {
        return MNEForwardSolution::read_forward_solution(p_sFile, fwd, force_fixed, surf_ori, include, exclude);
    }

    //=========================================================================================================
    /**
    * mne_read_forward_solution
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace::read_source_spaces static function
    *
    * Reads source spaces from a fif file
    *
    * @param [in] p_pFile   The open fiff file
    * @param [in] add_geom  Add geometry information to the source spaces
    * @param [in] p_pTree   Search for the source spaces here
    *
    * @param [out] p_pSourceSpace     The read source spaces
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_source_spaces(FiffFile*& p_pFile, bool add_geom, FiffDirTree*& p_pTree, MNESourceSpace*& p_pSourceSpace)
    {
        return MNESourceSpace::read_source_spaces(p_pFile, add_geom, p_pTree, p_pSourceSpace);
    }

    //=========================================================================================================
    /**
    * mne_transform_source_space_to
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace transform_source_space_to member function
    *
    * Transforms source space data to the desired coordinate system
    *
    * @param [in, out] p_pMNESourceSpace the source space which is should be transformed
    * @param [in] dest destination check code
    * @param [in] trans transformation information
    */
    static inline void transform_source_space_to(MNESourceSpace* p_pMNESourceSpace, fiff_int_t dest, FiffCoordTrans* trans)
    {
        return p_pMNESourceSpace->transform_source_space_to(dest, trans);
    }

    //=========================================================================================================
    /**
    * mne_transpose_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNEForwardSolution::transpose_named_matrix static function
    *
    * Transpose a named matrix (FiffNamedMatrix)
    *
    * @param[in, out] mat FiffNamedMatrix which shoul be transposed.
    *
    */
    static inline void transpose_named_matrix(FiffNamedMatrix*& mat)
    {
        return MNEForwardSolution::transpose_named_matrix(mat);
    }

private:



};


} // NAMESPACE

#endif // MNE_H
