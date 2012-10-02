//=============================================================================================================
/**
* @file     mne_forwardsolution.cpp
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
* @brief    ToDo Documentation...
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../include/mne_forwardsolution.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEForwardSolution::MNEForwardSolution()
: source_ori(0)
, coord_frame(0)
, nsource(0)
, nchan(0)
, sol(NULL)
, sol_grad(NULL)
, mri_head_t(FiffCoordTrans())
, src(NULL)
, source_rr(MatrixX3f::Zero(0,3))
, source_nn(MatrixX3f::Zero(0,3))
{

}


//*************************************************************************************************************

MNEForwardSolution::MNEForwardSolution(MNEForwardSolution* p_pMNEForwardSolution)
: source_ori(p_pMNEForwardSolution->source_ori)
, coord_frame(p_pMNEForwardSolution->coord_frame)
, nsource(p_pMNEForwardSolution->nsource)
, nchan(p_pMNEForwardSolution->nchan)
, sol(p_pMNEForwardSolution->sol ? new FiffNamedMatrix(p_pMNEForwardSolution->sol) : NULL)
, sol_grad(p_pMNEForwardSolution->sol_grad ? new FiffNamedMatrix(p_pMNEForwardSolution->sol_grad) : NULL)
, mri_head_t( p_pMNEForwardSolution->mri_head_t)
, src(p_pMNEForwardSolution->src ? new MNESourceSpace(p_pMNEForwardSolution->src) : NULL)
, source_rr(MatrixX3f(p_pMNEForwardSolution->source_rr))
, source_nn(MatrixX3f(p_pMNEForwardSolution->source_nn))
{

}


//*************************************************************************************************************

MNEForwardSolution::~MNEForwardSolution()
{
    if(sol)
        delete sol;
    if(sol_grad)
        delete sol_grad;
    if(src)
        delete src;
}


//*************************************************************************************************************

bool MNEForwardSolution::read_forward_solution(QString& p_sFileName, MNEForwardSolution*& fwd, bool force_fixed, bool surf_ori, QStringList& include, QStringList& exclude)
{
    FiffFile* t_pFile = new FiffFile(p_sFileName);
    FiffDirTree* t_pTree = NULL;
    QList<FiffDirEntry>* t_pDir = NULL;

    printf("Reading forward solution from %s...\n", p_sFileName.toUtf8().constData());
    if(!t_pFile->open(t_pTree, t_pDir))
        return false;

    //
    //   Find all forward solutions
    //
    QList<FiffDirTree*> fwds = t_pTree->dir_tree_find(FIFFB_MNE_FORWARD_SOLUTION);

    if (fwds.size() == 0)
    {
        t_pFile->close();
        std::cout << "No forward solutions in " << p_sFileName.toUtf8().constData(); // ToDo throw error
        //garbage collecting
        if(t_pDir)
            delete t_pDir;
        if(t_pTree)
            delete t_pTree;
        if(t_pFile)
            delete t_pFile;
        return false;
    }
    //
    //   Parent MRI data
    //
    QList<FiffDirTree*> parent_mri = t_pTree->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
    if (parent_mri.size() == 0)
    {
        t_pFile->close();
        std::cout << "No parent MRI information in " << p_sFileName.toUtf8().constData(); // ToDo throw error
        //garbage collecting
        if(t_pDir)
            delete t_pDir;
        if(t_pTree)
            delete t_pTree;
        if(t_pFile)
            delete t_pFile;
        return false;
    }

    MNESourceSpace* t_pSourceSpace = NULL;
    if(!MNESourceSpace::read_source_spaces(t_pFile, true, t_pTree, t_pSourceSpace))
    {
        t_pFile->close();
        std::cout << "Could not read the source spaces\n"; // ToDo throw error
        //ToDo error(me,'Could not read the source spaces (%s)',mne_omit_first_line(lasterr));
        //garbage collecting
        if(t_pDir)
            delete t_pDir;
        if(t_pTree)
            delete t_pTree;
        if(t_pFile)
            delete t_pFile;
        if(t_pSourceSpace)
            delete t_pSourceSpace;
        return false;
    }

    for(int k = 0; k < t_pSourceSpace->hemispheres.size(); ++k)
        t_pSourceSpace->hemispheres.at(k)->id = MNESourceSpace::find_source_space_hemi(t_pSourceSpace->hemispheres.at(k));

    //
    //   Bad channel list
    //
    QStringList bads = Fiff::read_bad_channels(t_pFile,t_pTree);
    printf("\t%d bad channels read\n",bads.size());
    qDebug() << bads;

    //
    //   Locate and read the forward solutions
    //
    FIFFLIB::FiffTag* t_pTag = NULL;
    FiffDirTree* megnode = NULL;
    FiffDirTree* eegnode = NULL;
    for(int k = 0; k < fwds.size(); ++k)
    {
        if(!fwds.at(k)->find_tag(t_pFile, FIFF_MNE_INCLUDED_METHODS, t_pTag))
        {
            t_pFile->close();
            std::cout << "Methods not listed for one of the forward solutions\n"; // ToDo throw error
            //garbage collecting
            if(t_pDir)
                delete t_pDir;
            if(t_pTree)
                delete t_pTree;
            if(t_pFile)
                delete t_pFile;
            if(t_pSourceSpace)
                delete t_pSourceSpace;
            return false;
        }
        if (*t_pTag->toInt() == FIFFV_MNE_MEG)
        {
            printf("MEG solution found\n");
            megnode = fwds.at(k);
        }
        else if(*t_pTag->toInt() == FIFFV_MNE_EEG)
        {
            printf("EEG solution found\n");
            eegnode = fwds.at(k);
        }
    }

    MNEForwardSolution* megfwd = NULL;
    QString ori;
    if (read_one(t_pFile, megnode, megfwd))
    {
        if (megfwd->source_ori == FIFFV_MNE_FIXED_ORI)
            ori = QString("fixed");
        else
            ori = QString("free");
        printf("\tRead MEG forward solution (%d sources, %d channels, %s orientations)\n", megfwd->nsource,megfwd->nchan,ori.toUtf8().constData());
    }
    MNEForwardSolution* eegfwd = NULL;
    if (read_one(t_pFile, eegnode, eegfwd))
    {
        if (eegfwd->source_ori == FIFFV_MNE_FIXED_ORI)
            ori = QString("fixed");
        else
            ori = QString("free");
        printf("\tRead EEG forward solution (%d sources, %d channels, %s orientations)\n", eegfwd->nsource,eegfwd->nchan,ori.toUtf8().constData());
    }

    //
    //   Merge the MEG and EEG solutions together
    //
    if (fwd != NULL)
        delete fwd;
    fwd = NULL;

    if (megfwd && eegfwd)
    {
        if (megfwd->sol->data.cols() != eegfwd->sol->data.cols() ||
                megfwd->source_ori != eegfwd->source_ori ||
                megfwd->nsource != eegfwd->nsource ||
                megfwd->coord_frame != eegfwd->coord_frame)
        {
            t_pFile->close();
            std::cout << "The MEG and EEG forward solutions do not match\n"; // ToDo throw error
            //garbage collecting
            if(t_pDir)
                delete t_pDir;
            if(t_pTree)
                delete t_pTree;
            if(t_pFile)
                delete t_pFile;
            if(t_pSourceSpace)
                delete t_pSourceSpace;
            return false;
        }

        fwd = new MNEForwardSolution(megfwd);
        fwd->sol->data.resize(megfwd->sol->nrow + eegfwd->sol->nrow, megfwd->sol->ncol);

        fwd->sol->data.block(0,0,megfwd->sol->nrow,megfwd->sol->ncol) = megfwd->sol->data;
        fwd->sol->data.block(megfwd->sol->nrow,0,eegfwd->sol->nrow,eegfwd->sol->ncol) = eegfwd->sol->data;
        fwd->sol->nrow = megfwd->sol->nrow + eegfwd->sol->nrow;
        fwd->sol->row_names.append(eegfwd->sol->row_names);

        if (fwd->sol_grad)
        {
            fwd->sol_grad->data.resize(megfwd->sol_grad->data.rows() + eegfwd->sol_grad->data.rows(), megfwd->sol_grad->data.cols());

            fwd->sol->data.block(0,0,megfwd->sol_grad->data.rows(),megfwd->sol_grad->data.cols()) = megfwd->sol_grad->data;
            fwd->sol->data.block(megfwd->sol_grad->data.rows(),0,eegfwd->sol_grad->data.rows(),eegfwd->sol_grad->data.cols()) = eegfwd->sol_grad->data;

            fwd->sol_grad->nrow      = megfwd->sol_grad->nrow + eegfwd->sol_grad->nrow;
            fwd->sol_grad->row_names.append(eegfwd->sol_grad->row_names);
        }
        fwd->nchan  = megfwd->nchan + eegfwd->nchan;
        printf("\tMEG and EEG forward solutions combined\n");
    }
    else if (megfwd)
        fwd = megfwd; //new MNEForwardSolution(megfwd);//not copied for the sake of speed
    else
        fwd = eegfwd; //new MNEForwardSolution(eegfwd);//not copied for the sake of speed

    //
    //   Get the MRI <-> head coordinate transformation
    //
    if(!parent_mri.at(0)->find_tag(t_pFile, FIFF_COORD_TRANS, t_pTag))
    {
        t_pFile->close();
        std::cout << "MRI/head coordinate transformation not found\n"; // ToDo throw error
        //garbage collecting
        if(t_pDir)
            delete t_pDir;
        if(t_pTree)
            delete t_pTree;
        if(t_pFile)
            delete t_pFile;
        if(t_pSourceSpace)
            delete t_pSourceSpace;
        if(t_pTag)
            delete t_pTag;
        return false;
    }
    else
    {
        fwd->mri_head_t = t_pTag->toCoordTrans();
//            std::cout << "Transformation\n" << fwd->mri_head_t.trans << std::endl;
//            std::cout << "from " << fwd->mri_head_t.from << " to " << fwd->mri_head_t.to << std::endl;

//            std::cout << "inverse Transformation\n" << fwd->mri_head_t.invtrans << std::endl;
//            std::cout << "size " << sizeof(FiffCoordTrans) << std::endl;
//            std::cout << "size " << sizeof(fiffCoordTransRec) << std::endl;

        if (fwd->mri_head_t.from != FIFFV_COORD_MRI || fwd->mri_head_t.to != FIFFV_COORD_HEAD)
        {
            FiffCoordTrans::invert_transform(&fwd->mri_head_t);
            if (fwd->mri_head_t.from != FIFFV_COORD_MRI || fwd->mri_head_t.to != FIFFV_COORD_HEAD)
            {
                t_pFile->close();
                std::cout << "MRI/head coordinate transformation not found\n"; // ToDo throw error
                //garbage collecting
                if(t_pDir)
                    delete t_pDir;
                if(t_pTree)
                    delete t_pTree;
                if(t_pFile)
                    delete t_pFile;
                if(t_pSourceSpace)
                    delete t_pSourceSpace;
                if(t_pTag)
                    delete t_pTag;
                return false;
            }
        }
    }
    t_pFile->close();

    //
    //   Transform the source spaces to the correct coordinate frame
    //   if necessary
    //
    if (fwd->coord_frame != FIFFV_COORD_MRI && fwd->coord_frame != FIFFV_COORD_HEAD)
    {
        std::cout << "Only forward solutions computed in MRI or head coordinates are acceptable";
        return false;
    }

    //
    qint32 nuse = 0;
    t_pSourceSpace->transform_source_space_to(fwd->coord_frame,&fwd->mri_head_t);
    for(int k = 0; k < t_pSourceSpace->hemispheres.size(); ++k)
        nuse = nuse +  t_pSourceSpace->hemispheres.at(k)->nuse;

    if (nuse != fwd->nsource)
        throw("Source spaces do not match the forward solution.\n");

    printf("\tSource spaces transformed to the forward solution coordinate frame\n");
    fwd->src = t_pSourceSpace; //not new MNESourceSpace(t_pSourceSpace); for sake of speed
    //
    //   Handle the source locations and orientations
    //
    if (fwd->source_ori == FIFFV_MNE_FIXED_ORI || force_fixed == true)
    {
        nuse = 0;
        fwd->source_rr = MatrixXf::Zero(fwd->nsource,3);
        fwd->source_nn = MatrixXf::Zero(fwd->nsource,3);
        for(qint32 k = 0; k < t_pSourceSpace->hemispheres.size();++k)
        {
            for(qint32 q = 0; q < t_pSourceSpace->hemispheres.at(k)->nuse; ++q)
            {
                fwd->source_rr.block(q,0,1,3) = t_pSourceSpace->hemispheres.at(k)->rr.block(t_pSourceSpace->hemispheres.at(k)->vertno(q),0,1,3);
                fwd->source_nn.block(q,0,1,3) = t_pSourceSpace->hemispheres.at(k)->nn.block(t_pSourceSpace->hemispheres.at(k)->vertno(q),0,1,3);
            }
            nuse = nuse + t_pSourceSpace->hemispheres.at(k)->nuse;
        }
        //
        //   Modify the forward solution for fixed source orientations
        //
        if (fwd->source_ori != FIFFV_MNE_FIXED_ORI)
        {
            printf("\tChanging to fixed-orientation forward solution...");

            MatrixXf tmp = fwd->source_nn.transpose();

            SparseMatrix<float> fix_rot = make_block_diag(tmp,1);
            fwd->sol->data  = fwd->sol->data*fix_rot;
            fwd->sol->ncol  = fwd->nsource;
            fwd->source_ori = FIFFV_MNE_FIXED_ORI;

            if (fwd->sol_grad != NULL)
            {
                SparseMatrix<float> t_matKron;
                SparseMatrix<float> t_eye(3,3);
                for (qint32 i = 0; i < 3; ++i)
                    t_eye.insert(i,i) = 1.0f;
                kroneckerProduct(fix_rot,t_eye,t_matKron);//kron(fix_rot,eye(3));
                fwd->sol_grad->data   = fwd->sol_grad->data*t_matKron;
                fwd->sol_grad->ncol   = 3*fwd->nsource;
            }
            printf("[done]\n");
        }
    }
    else
    {
        if (surf_ori)
        {
            //
            //   Rotate the local source coordinate systems
            //
            printf("\tConverting to surface-based source orientations...");
            nuse = 0;
            qint32 pp = 0;
            fwd->source_rr = MatrixXf::Zero(fwd->nsource,3);
            fwd->source_nn = MatrixXf::Zero(fwd->nsource*3,3);

            printf(" (Warning: Rotating the source coordinate system haven't been verified --> Singular Vectors U are different from MATLAB!) ");

            for(qint32 k = 0; k < t_pSourceSpace->hemispheres.size();++k)
            {

                for (qint32 q = 0; q < t_pSourceSpace->hemispheres.at(k)->nuse; ++q)
                    fwd->source_rr.block(q+nuse,0,1,3) = t_pSourceSpace->hemispheres.at(k)->rr.block(t_pSourceSpace->hemispheres.at(k)->vertno(q),0,1,3);

                for (qint32 p = 0; p < t_pSourceSpace->hemispheres.at(k)->nuse; ++p)
                {
                    //
                    //  Project out the surface normal and compute SVD
                    //
                    Vector3f nn = t_pSourceSpace->hemispheres.at(k)->nn.block(t_pSourceSpace->hemispheres.at(k)->vertno(p),0,1,3).transpose();

                    Matrix3f tmp = Matrix3f::Identity() - nn*nn.transpose();

                    JacobiSVD<MatrixXf> t_svd(tmp, Eigen::ComputeThinU);

                    MatrixXf U(t_svd.matrixU());

                    //
                    //  Make sure that ez is in the direction of nn
                    //
                    if ((nn.transpose() * U.block(0,2,3,1))(0,0) < 0)
                    {
                        U *= -1;
                    }

                    fwd->source_nn.block(pp, 0, 3, 3) = U.transpose();

                    pp += 3;
                }
                nuse += t_pSourceSpace->hemispheres.at(k)->nuse;
            }
            MatrixXf tmp = fwd->source_nn.transpose();

            SparseMatrix<float> surf_rot = make_block_diag(tmp,3);

            fwd->sol->data = fwd->sol->data*surf_rot;

            if (fwd->sol_grad != NULL)
            {
                SparseMatrix<float> t_matKron;
                SparseMatrix<float> t_eye(3,3);
                for (qint32 i = 0; i < 3; ++i)
                    t_eye.insert(i,i) = 1.0f;
                kroneckerProduct(surf_rot,t_eye,t_matKron);//kron(surf_rot,eye(3));
                fwd->sol_grad->data   = fwd->sol_grad->data*t_matKron;
            }

            printf("[done]\n");
        }
        else
        {
            printf("\tCartesian source orientations...");
            nuse = 0;
            fwd->source_rr = MatrixXf::Zero(fwd->nsource,3);
            for(int k = 0; k < t_pSourceSpace->hemispheres.size(); ++k)
            {
                for (qint32 q = 0; q < t_pSourceSpace->hemispheres.at(k)->nuse; ++q)
                    fwd->source_rr.block(q+nuse,0,1,3) = t_pSourceSpace->hemispheres.at(k)->rr.block(t_pSourceSpace->hemispheres.at(k)->vertno(q),0,1,3);

                nuse += t_pSourceSpace->hemispheres.at(k)->nuse;
            }

            MatrixXf t_ones = MatrixXf::Ones(fwd->nsource,1);
            Matrix3f t_eye = Matrix3f::Identity();
            kroneckerProduct(t_ones,t_eye,fwd->source_nn);

            printf("[done]\n");
        }
    }

    //
    //   Do the channel selection
    //
    if (include.size() > 0 || exclude.size() > 0 || bads.size() > 0)
    {
        //
        //   First do the channels to be included
        //
        RowVectorXi pick;
        if (include.size() == 0)
            pick = RowVectorXi::Ones(fwd->nchan);
        else
        {
            pick = RowVectorXi::Zero(fwd->nchan);
            for(qint32 k = 0; k < include.size(); ++k)
            {
                QList<int> c;
                for(qint32 l = 0; l < fwd->sol->row_names.size(); ++l)
                    if(fwd->sol->row_names.at(l).contains(include.at(k),Qt::CaseInsensitive))
                        pick(l) = 1;

//                    c = strmatch(include{k},fwd.sol.row_names,'exact');
//                    for p = 1:length(c)
//                        pick(c(p)) = 1;
//                    end
            }
        }
        //
        //   Then exclude what needs to be excluded
        //
        if (exclude.size() > 0)
        {
            for(qint32 k = 0; k < exclude.size(); ++k)
            {
                QList<int> c;
                for(qint32 l = 0; l < fwd->sol->row_names.size(); ++l)
                    if(fwd->sol->row_names.at(l).contains(exclude.at(k),Qt::CaseInsensitive))
                        pick(l) = 0;

//                    c = strmatch(exclude{k},fwd.sol.row_names,'exact');
//                    for p = 1:length(c)
//                        pick(c(p)) = 0;
//                    end
            }
        }
        if (bads.size() > 0)
        {
            for(qint32 k = 0; k < bads.size(); ++k)
            {
                QList<int> c;
                for(qint32 l = 0; l < fwd->sol->row_names.size(); ++l)
                    if(fwd->sol->row_names.at(l).contains(bads.at(k),Qt::CaseInsensitive))
                        pick(l) = 0;

//                    c = strmatch(bads{k},fwd.sol.row_names,'exact');
//                    for p = 1:length(c)
//                        pick(c(p)) = 0;
//                    end
            }
        }
        //
        //   Do we have something?
        //
        nuse = pick.sum();
        if (nuse == 0)
            throw("Nothing remains after picking");
        //
        //   Create the selector
        //
        qint32 p = 0;
        MatrixXf t_solData(nuse,fwd->sol->data.cols());
        QStringList t_solRowNames;

        MatrixXf t_solGradData;
        QStringList t_solGradRowNames;

        if (fwd->sol_grad != NULL)
            t_solGradData.resize(nuse, fwd->sol_grad->data.cols());

        for(qint32 k = 0; k < fwd->nchan; ++k)
        {
            if(pick(k) > 0)
            {
                t_solData.block(p, 0, 1, fwd->sol->data.cols()) = fwd->sol->data.block(k, 0, 1, fwd->sol->data.cols());
                t_solRowNames.append(fwd->sol->row_names.at(k));

                if (fwd->sol_grad != NULL)
                {
                    t_solGradData.block(p, 0, 1, fwd->sol_grad->data.cols()) = fwd->sol_grad->data.block(k, 0, 1, fwd->sol_grad->data.cols());
                    t_solGradRowNames.append(fwd->sol_grad->row_names.at(k));
                }

                ++p;
            }
        }
        printf("\t%d out of %d channels remain after picking\n",nuse,fwd->nchan);
        //
        //   Pick the correct rows of the forward operator
        //
        fwd->nchan          = nuse;
        fwd->sol->data      = t_solData;
        fwd->sol->nrow      = nuse;
        fwd->sol->row_names = t_solRowNames;

        if (fwd->sol_grad != NULL)
        {
            fwd->sol_grad->data      = t_solGradData;
            fwd->sol_grad->nrow      = nuse;
            fwd->sol_grad->row_names = t_solGradRowNames;
        }
    }

    //garbage collecting
    t_pFile->close();

    //garbage collecting
//    if (megfwd)
//        delete megfwd; // don't delete the megfwd because fwd->src is pointing to source space (not copied for the sake of speed)
//    if (eegfwd)
//        delete eegfwd; // don't delete the eegfwd because fwd->src is pointing to source space (not copied for the sake of speed)
    if(t_pDir)
        delete t_pDir;
    if(t_pTree)
        delete t_pTree;
    if(t_pFile)
        delete t_pFile;
//    if(t_pSourceSpace)
//        delete t_pSourceSpace; // don't delete the SourceSpace because fwd->src is pointing to source space (not copied for the sake of speed)
    if(t_pTag)
        delete t_pTag;

    return true;
}


//*************************************************************************************************************

//    static inline MatrixXf extract_block_diag(MatrixXf& A, qint32 n)
//    {


//        //
//        // Principal Investigators and Developers:
//        // ** Richard M. Leahy, PhD, Signal & Image Processing Institute,
//        //    University of Southern California, Los Angeles, CA
//        // ** John C. Mosher, PhD, Biophysics Group,
//        //    Los Alamos National Laboratory, Los Alamos, NM
//        // ** Sylvain Baillet, PhD, Cognitive Neuroscience & Brain Imaging Laboratory,
//        //    CNRS, Hopital de la Salpetriere, Paris, France
//        //
//        // Copyright (c) 2005 BrainStorm by the University of Southern California
//        // This software distributed  under the terms of the GNU General Public License
//        // as published by the Free Software Foundation. Further details on the GPL
//        // license can be found at http://www.gnu.org/copyleft/gpl.html .
//        //
//        //FOR RESEARCH PURPOSES ONLY. THE SOFTWARE IS PROVIDED "AS IS," AND THE
//        // UNIVERSITY OF SOUTHERN CALIFORNIA AND ITS COLLABORATORS DO NOT MAKE ANY
//        // WARRANTY, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
//        // MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, NOR DO THEY ASSUME ANY
//        // LIABILITY OR RESPONSIBILITY FOR THE USE OF THIS SOFTWARE.
//        //
//        // Author: John C. Mosher 1993 - 2004
//        //
//        //
//        // Modifications for mne Matlab toolbox
//        //
//        //   Matti Hamalainen
//        //   2006


//          [mA,na] = size(A);		% matrix always has na columns
//          % how many entries in the first column?
//          bdn = na/n;			% number of blocks
//          ma = mA/bdn;			% rows in first block

//          % blocks may themselves contain zero entries.  Build indexing as above
//          tmp = reshape([1:(ma*bdn)]',ma,bdn);
//          i = zeros(ma*n,bdn);
//          for iblock = 1:n,
//            i((iblock-1)*ma+[1:ma],:) = tmp;
//          end

//          i = i(:); 			% row indices foreach sparse bd


//          j = [0:mA:(mA*(na-1))];
//          j = j(ones(ma,1),:);
//          j = j(:);

//          i = i + j;

//          bd = full(A(i)); 	% column vector
//          bd = reshape(bd,ma,na);	% full matrix

//    }


//*************************************************************************************************************

inline SparseMatrix<float> MNEForwardSolution::make_block_diag(MatrixXf& A, qint32 n)
{

    float ma = A.rows();
    float na = A.cols();
    float bdn = na/n; 			// number of submatrices

    std::cout << std::endl << "ma " << ma << " na " << na << " bdn " << bdn << std::endl;

    if(bdn - floor(bdn))
        throw("Width of matrix must be even multiple of n");

    SparseMatrix<float> bd(ma*bdn,na);

    qint32 current_col, current_row, i, r, c;
    MatrixXf matblock;
    for(i = 0; i < bdn; ++i)
    {
        current_col = i * n;
        current_row = i * ma;
        matblock = A.block(0, current_col, ma, n);

        for(r = 0; r < ma; ++r)
            for(c = 0; c < n; ++c)
                bd.insert(r+current_row,c+current_col)=A(r, c+current_col);
    }
    return bd;
}


//*************************************************************************************************************

bool MNEForwardSolution::read_one(FiffFile* p_pFile, FiffDirTree* node, MNEForwardSolution*& one)
{
    if (one != NULL)
        delete one;
    //
    //   Read all interesting stuff for one forward solution
    //
    if(node == NULL)
    {
        one = NULL;
        return false;
    }

    one = new MNEForwardSolution();
    FIFFLIB::FiffTag* t_pTag = NULL;

    if(!node->find_tag(p_pFile, FIFF_MNE_SOURCE_ORIENTATION, t_pTag))
    {
        p_pFile->close();
        std::cout << "Source orientation tag not found."; //ToDo: throw error.
        return false;
    }

    one->source_ori = *t_pTag->toInt();

    if(!node->find_tag(p_pFile, FIFF_MNE_COORD_FRAME, t_pTag))
    {
        p_pFile->close();
        std::cout << "Coordinate frame tag not found."; //ToDo: throw error.
        return false;
    }

    one->coord_frame = *t_pTag->toInt();

    if(!node->find_tag(p_pFile, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag))
    {
        p_pFile->close();
        std::cout << "Number of sources not found."; //ToDo: throw error.
        return false;
    }

    one->nsource = *t_pTag->toInt();

    if(!node->find_tag(p_pFile, FIFF_NCHAN, t_pTag))
    {
        p_pFile->close();
        printf("Number of channels not found."); //ToDo: throw error.
        return false;
    }

    one->nchan = *t_pTag->toInt();

    if(node->read_named_matrix(p_pFile, FIFF_MNE_FORWARD_SOLUTION, one->sol))
        transpose_named_matrix(one->sol);
    else
    {
        p_pFile->close();
        printf("Forward solution data not found ."); //ToDo: throw error.
        //error(me,'Forward solution data not found (%s)',mne_omit_first_line(lasterr));
        return false;
    }

    if(node->read_named_matrix(p_pFile, FIFF_MNE_FORWARD_SOLUTION_GRAD, one->sol_grad))
        transpose_named_matrix(one->sol_grad);
    else
    {
        if (one->sol_grad)
            delete one->sol_grad;
        one->sol_grad = NULL;
    }


    if (one->sol->data.rows() != one->nchan ||
            (one->sol->data.cols() != one->nsource && one->sol->data.cols() != 3*one->nsource))
    {
        p_pFile->close();
        printf("Forward solution matrix has wrong dimensions.\n"); //ToDo: throw error.
        //error(me,'Forward solution matrix has wrong dimensions');
        return false;
    }
    if (one->sol_grad)
    {
        if (one->sol_grad->data.rows() != one->nchan ||
                (one->sol_grad->data.cols() != 3*one->nsource && one->sol_grad->data.cols() != 3*3*one->nsource))
        {
            p_pFile->close();
            printf("Forward solution gradient matrix has wrong dimensions.\n"); //ToDo: throw error.
            //error(me,'Forward solution gradient matrix has wrong dimensions');
        }
    }
    if (t_pTag)
        delete t_pTag;
    return true;
}


//*************************************************************************************************************

void MNEForwardSolution::transpose_named_matrix(FiffNamedMatrix*& mat)
{
    QStringList col_names_old = mat->col_names;
    mat->col_names = mat->row_names;
    mat->row_names = col_names_old;

    MatrixXf tmp_data = mat->data.transpose();

    mat->data = tmp_data;

    mat->nrow = mat->data.rows();
    mat->ncol = mat->data.cols();
}
