//=============================================================================================================
/**
* @file     mne_forwardsolution.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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

#include "mne_forwardsolution.h"
#include "mne_math.h"


//*************************************************************************************************************
//=============================================================================================================
// FS INCLUDES
//=============================================================================================================

#include "../fs/colortable.h"


//*************************************************************************************************************
//=============================================================================================================
// MNE_MATH INCLUDES
//=============================================================================================================

#include "../mnemath/kmeans.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace MNEMATHLIB;
using namespace FSLIB;


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
, mri_head_t(NULL)
, src(NULL)
, source_rr(MatrixX3d::Zero(0,3))
, source_nn(MatrixX3d::Zero(0,3))
, isClustered(false)
{

}


//*************************************************************************************************************

MNEForwardSolution::MNEForwardSolution(const MNEForwardSolution* p_pMNEForwardSolution)
: source_ori(p_pMNEForwardSolution->source_ori)
, coord_frame(p_pMNEForwardSolution->coord_frame)
, nsource(p_pMNEForwardSolution->nsource)
, nchan(p_pMNEForwardSolution->nchan)
, sol(p_pMNEForwardSolution->sol ? new FiffNamedMatrix(p_pMNEForwardSolution->sol) : NULL)
, sol_grad(p_pMNEForwardSolution->sol_grad ? new FiffNamedMatrix(p_pMNEForwardSolution->sol_grad) : NULL)
, mri_head_t( new FiffCoordTrans(p_pMNEForwardSolution->mri_head_t) )
, src(p_pMNEForwardSolution->src ? new MNESourceSpace(p_pMNEForwardSolution->src) : NULL)
, source_rr(MatrixX3d(p_pMNEForwardSolution->source_rr))
, source_nn(MatrixX3d(p_pMNEForwardSolution->source_nn))
, isClustered(p_pMNEForwardSolution->isClustered)
{

}


//*************************************************************************************************************

MNEForwardSolution::~MNEForwardSolution()
{
    if(sol)
        delete sol;
    if(sol_grad)
        delete sol_grad;
    if(mri_head_t)
        delete mri_head_t;
    if(src)
        delete src;
}


//*************************************************************************************************************

bool MNEForwardSolution::cluster_forward_solution(MNEForwardSolution *p_fwdOut, Annotation* p_pLHAnnotation, Annotation* p_pRHAnnotation, qint32 p_iClusterSize)
{
    if(p_fwdOut)
        delete p_fwdOut;
    p_fwdOut = new MNEForwardSolution(this);

    QList<Annotation*> t_listAnnotation;
    t_listAnnotation.append(p_pLHAnnotation);
    t_listAnnotation.append(p_pRHAnnotation);

    //
    // Check consisty
    //
//    for(qint32 h = 0; h < this->src->hemispheres.size(); ++h )//obj.sizeForwardSolution)
//    {
//        if(this->src->hemispheres[h]->vertno.rows() !=  t_listAnnotation[h]->getLabel()->rows())
//        {
//            printf("Error: Annotation doesn't fit to Forward Solution: Vertice number is different!");
//            return false;
//        }
//    }


//    //DEBUG
//    MatrixXd test(20,3);
//    test << 0.537667139546100, 1.83388501459509, -2.25884686100365,
//            0.862173320368121, 0.318765239858981, -1.30768829630527,
//            -0.433592022305684, 0.342624466538650, 3.57839693972576,
//            2.76943702988488, -1.34988694015652, 3.03492346633185,
//            0.725404224946106, -0.0630548731896562, 0.714742903826096,
//            -0.204966058299775, -0.124144348216312, 1.48969760778547,
//            1.40903448980048, 1.41719241342961, 0.671497133608081,
//            -1.20748692268504, 0.717238651328839, 1.63023528916473,
//            0.488893770311789, 1.03469300991786, 0.726885133383238,
//            -0.303440924786016, 0.293871467096658, -0.787282803758638,
//            0.888395631757642, -1.14707010696915, -1.06887045816803,
//            -0.809498694424876, -2.94428416199490, 1.43838029281510,
//            0.325190539456198, -0.754928319169703, 1.37029854009523,
//            -1.71151641885370, -0.102242446085491, -0.241447041607358,
//            0.319206739165502, 0.312858596637428, -0.864879917324457,
//            -0.0300512961962686, -0.164879019209038, 0.627707287528727,
//            1.09326566903948, 1.10927329761440, -0.863652821988714,
//            0.0773590911304249, -1.21411704361541, -1.11350074148676,
//            -0.00684932810334806, 1.53263030828475, -0.769665913753682,
//            0.371378812760058, -0.225584402271252, 1.11735613881447;

//    std::cout << test << std::endl;

//    VectorXi idx;
//    MatrixXd ctrs;
//    VectorXd sumd;
//    MatrixXd D;

//    KMeans testK(QString("cityblock"), QString("sample"), 5);//QString("sqeuclidean")//QString("sample")

//    testK.calculate(test, 2, idx, ctrs, sumd, D);

//    std::cout << "idx" << std::endl << idx << std::endl;
//    std::cout << "ctrs" << std::endl << ctrs << std::endl;
//    std::cout << "sumd" << std::endl << sumd << std::endl;
//    std::cout << "D" << std::endl << D << std::endl;


    //DEBUG END


    KMeans t_kMeans(QString("sqeuclidean"), QString("sample"), 5);//QString("sqeuclidean")//QString("sample")//cityblock
    MatrixXd t_LF_new;

    qint32 count;
    qint32 offset;

    for(qint32 h = 0; h < this->src->hemispheres.size(); ++h )//obj.sizeForwardSolution)
    {
        count = 0;
        offset = 0;

        // Offset for continuous indexing;
        if(h > 0)
            for(qint32 j = 0; j < h; ++j)
                offset += this->src->hemispheres[j]->nuse;


        if(h == 0)
            printf("Cluster Left Hemisphere\n");
        else
            printf("Cluster Right Hemisphere\n");

        Colortable* t_pCurrentColorTable = t_listAnnotation[h]->getColortable();
        VectorXi label = t_pCurrentColorTable->getAvailableROIs();

        // Get label for every vertice
        VectorXi vertno_labeled = VectorXi::Zero(this->src->hemispheres[h]->vertno.rows());

        for(qint32 i = 0; i < vertno_labeled.rows(); ++i)
            vertno_labeled[i] = (*t_listAnnotation[h]->getLabel())[this->src->hemispheres[h]->vertno[i]];

        MatrixXd t_LF_partial;
        for (qint32 i = 0; i < label.rows(); ++i)
        {

            if (label[i] != 0)
            {
                QString curr_name = t_pCurrentColorTable->struct_names[i];//obj.label2AtlasName(label(i));
                printf("\tCluster %d / %d %s...", i+1, label.rows(), curr_name.toUtf8().constData());

                //
                // Get source space indeces
                //
                VectorXi idcs = VectorXi::Zero(vertno_labeled.rows());
                qint32 c = 0;

                //Select ROIs
                for(qint32 j = 0; j < vertno_labeled.rows(); ++j)
                {
                    if(vertno_labeled[j] == label[i])
                    {
                        idcs[c] = j;
                        ++c;
                    }
                }
                idcs.conservativeResize(c);

//                VectorXi idcs_triplet = tripletSelection(idcs);//ToDo obsolete: use block instead

                //get selected LF
                MatrixXd t_LF(this->sol->data->rows(), idcs.rows()*3);

                for(qint32 j = 0; j < idcs.rows(); ++j)
                    t_LF.block(0, j*3, t_LF.rows(), 3) = this->sol->data->block(0, (idcs[j]+offset)*3, t_LF.rows(), 3);

                qint32 nSens = t_LF.rows();
                qint32 nSources = t_LF.cols()/3;
                qint32 nClusters = 0;

                if (nSources > 0)
                {
                    nClusters = ceil((double)nSources/(double)p_iClusterSize);

                    printf("%d Cluster(s)... ", nClusters);

                    t_LF_partial = MatrixXd::Zero(nSens,nClusters*3);





                    // Reshape Input data -> sources rows; sensors columns
                    MatrixXd t_sensLF(t_LF.cols()/3, 3*nSens);
                    for(qint32 j = 0; j < nSens; ++j)
                    {
                        for(qint32 k = 0; k < t_sensLF.rows(); ++k)
                            t_sensLF.block(k,j*3,1,3) = t_LF.block(j,k*3,1,3);
                    }

                    // Kmeans Reduction
                    VectorXi roiIdx;
                    MatrixXd ctrs;
                    VectorXd sumd;
                    MatrixXd D;

                    t_kMeans.calculate(t_sensLF, nClusters, roiIdx, ctrs, sumd, D);

                    //
                    // Assign the centroid for each cluster to the partial LF
                    //
                    for(qint32 j = 0; j < nSens; ++j)
                        for(qint32 k = 0; k < nClusters; ++k)
                            t_LF_partial.block(j, k*3, 1, 3) = ctrs.block(k,j*3,1,3);

                    //
                    // Get cluster indizes and its distances to the centroid
                    //
                    for(qint32 j = 0; j < nClusters; ++j)
                    {
                        VectorXi clusterIdcs = VectorXi::Zero(roiIdx.rows());
                        VectorXd clusterDistance = VectorXd::Zero(roiIdx.rows());
                        qint32 nClusterIdcs = 0;
                        for(qint32 k = 0; k < roiIdx.rows(); ++k)
                        {
                            if(roiIdx[k] == j)
                            {
                                clusterIdcs[nClusterIdcs] = idcs[k];
                                clusterDistance[nClusterIdcs] = D(k,j);
                                ++nClusterIdcs;
                            }
                        }
                        clusterIdcs.conservativeResize(nClusterIdcs);
                        p_fwdOut->src->hemispheres[h]->cluster_vertnos.append(clusterIdcs);
                        p_fwdOut->src->hemispheres[h]->cluster_distances.append(clusterDistance);
                    }

                    //
                    // Assign partial LF to new LeadField
                    //
                    if(t_LF_partial.rows() > 0 && t_LF_partial.cols() > 0)
                    {
                        t_LF_new.conservativeResize(t_LF_partial.rows(), t_LF_new.cols() + t_LF_partial.cols());
                        t_LF_new.block(0, t_LF_new.cols() - t_LF_partial.cols(), t_LF_new.rows(), t_LF_partial.cols()) = t_LF_partial;

                        // Map the centroids to the closest rr
                        for(qint32 k = 0; k < nClusters; ++k)
                        {
                            qint32 j = 0;

                            double sqec = sqrt((t_LF.block(0, j*3, t_LF.rows(), 3) - t_LF_partial.block(0, k*3, t_LF_partial.rows(), 3)).array().pow(2).sum());
                            double sqec_min = sqec;
                            double j_min = j;
                            for(qint32 j = 1; j < idcs.rows(); ++j)
                            {
                                sqec = sqrt((t_LF.block(0, j*3, t_LF.rows(), 3) - t_LF_partial.block(0, k*3, t_LF_partial.rows(), 3)).array().pow(2).sum());
                                if(sqec < sqec_min)
                                {
                                    sqec_min = sqec;
                                    j_min = j;
                                }
                            }

                            // Take the closest coordinates
                            qint32 sel_idx = idcs[j_min];
                            p_fwdOut->src->hemispheres[h]->rr.row(count) = this->src->hemispheres[h]->rr.row(sel_idx);
                            p_fwdOut->src->hemispheres[h]->nn.row(count) = MatrixXd::Zero(1,3);

                            p_fwdOut->src->hemispheres[h]->vertno[count] = this->src->hemispheres[h]->vertno[sel_idx];
                            ++count;
                        }
                    }
                    printf("[done]\n");
                }
                else
                {
                    printf("failed! Label contains no sources.\n");
                }
            }
        }

        p_fwdOut->src->hemispheres[h]->rr.conservativeResize(count, 3);
        p_fwdOut->src->hemispheres[h]->nn.conservativeResize(count, 3);
        p_fwdOut->src->hemispheres[h]->vertno.conservativeResize(count);

        p_fwdOut->src->hemispheres[h]->nuse_tri = 0;
        p_fwdOut->src->hemispheres[h]->use_tris = MatrixX3i(0,3);


        if(p_fwdOut->src->hemispheres[h]->rr.rows() > 0 && p_fwdOut->src->hemispheres[h]->rr.cols() > 0)
        {
            if(h == 0)
            {
                p_fwdOut->source_rr = MatrixX3d(0,3);
                p_fwdOut->source_nn = MatrixX3d(0,3);
            }

            p_fwdOut->source_rr.conservativeResize(p_fwdOut->source_rr.rows() + p_fwdOut->src->hemispheres[h]->rr.rows(),3);
            p_fwdOut->source_rr.block(p_fwdOut->source_rr.rows() -  p_fwdOut->src->hemispheres[h]->rr.rows(), 0,  p_fwdOut->src->hemispheres[h]->rr.rows(), 3) = p_fwdOut->src->hemispheres[h]->rr;

            p_fwdOut->source_nn.conservativeResize(p_fwdOut->source_nn.rows() + p_fwdOut->src->hemispheres[h]->nn.rows(),3);
            p_fwdOut->source_nn.block(p_fwdOut->source_nn.rows() -  p_fwdOut->src->hemispheres[h]->nn.rows(), 0,  p_fwdOut->src->hemispheres[h]->nn.rows(), 3) = p_fwdOut->src->hemispheres[h]->nn;
        }

        printf("[done]\n");
    }

    // set new fwd solution;
    *(p_fwdOut->sol->data) = t_LF_new;
    p_fwdOut->sol->ncol = t_LF_new.cols();

    p_fwdOut->nsource = p_fwdOut->sol->ncol/3;

    p_fwdOut->isClustered = true;

    return true;
}


//*************************************************************************************************************

bool MNEForwardSolution::read_forward_solution(QIODevice* p_pIODevice, MNEForwardSolution*& fwd, bool force_fixed, bool surf_ori, QStringList& include, QStringList& exclude)
{
    FiffStream* t_pStream = new FiffStream(p_pIODevice);
    FiffDirTree* t_pTree = NULL;
    QList<FiffDirEntry>* t_pDir = NULL;

    printf("Reading forward solution from %s...\n", t_pStream->streamName().toUtf8().constData());
    if(!t_pStream->open(t_pTree, t_pDir))
        return false;

    //
    //   Find all forward solutions
    //
    QList<FiffDirTree*> fwds = t_pTree->dir_tree_find(FIFFB_MNE_FORWARD_SOLUTION);

    if (fwds.size() == 0)
    {
        t_pStream->device()->close();
        std::cout << "No forward solutions in " << t_pStream->streamName().toUtf8().constData(); // ToDo throw error
        //garbage collecting
        if(t_pDir)
            delete t_pDir;
        if(t_pTree)
            delete t_pTree;
        if(t_pStream)
            delete t_pStream;
        return false;
    }
    //
    //   Parent MRI data
    //
    QList<FiffDirTree*> parent_mri = t_pTree->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
    if (parent_mri.size() == 0)
    {
        t_pStream->device()->close();
        std::cout << "No parent MRI information in " << t_pStream->streamName().toUtf8().constData(); // ToDo throw error
        //garbage collecting
        if(t_pDir)
            delete t_pDir;
        if(t_pTree)
            delete t_pTree;
        if(t_pStream)
            delete t_pStream;
        return false;
    }

    MNESourceSpace* t_pSourceSpace = NULL;
    if(!MNESourceSpace::read_source_spaces(t_pStream, true, t_pTree, t_pSourceSpace))
    {
        t_pStream->device()->close();
        std::cout << "Could not read the source spaces\n"; // ToDo throw error
        //ToDo error(me,'Could not read the source spaces (%s)',mne_omit_first_line(lasterr));
        //garbage collecting
        if(t_pDir)
            delete t_pDir;
        if(t_pTree)
            delete t_pTree;
        if(t_pStream)
            delete t_pStream;
        if(t_pSourceSpace)
            delete t_pSourceSpace;
        return false;
    }

    for(int k = 0; k < t_pSourceSpace->hemispheres.size(); ++k)
        t_pSourceSpace->hemispheres.at(k)->id = MNESourceSpace::find_source_space_hemi(t_pSourceSpace->hemispheres.at(k));

    //
    //   Bad channel list
    //
    QStringList bads = Fiff::read_bad_channels(t_pStream,t_pTree);
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
        if(!fwds.at(k)->find_tag(t_pStream, FIFF_MNE_INCLUDED_METHODS, t_pTag))
        {
            t_pStream->device()->close();
            std::cout << "Methods not listed for one of the forward solutions\n"; // ToDo throw error
            //garbage collecting
            if(t_pDir)
                delete t_pDir;
            if(t_pTree)
                delete t_pTree;
            if(t_pStream)
                delete t_pStream;
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
    if (read_one(t_pStream, megnode, megfwd))
    {
        if (megfwd->source_ori == FIFFV_MNE_FIXED_ORI)
            ori = QString("fixed");
        else
            ori = QString("free");
        printf("\tRead MEG forward solution (%d sources, %d channels, %s orientations)\n", megfwd->nsource,megfwd->nchan,ori.toUtf8().constData());
    }
    MNEForwardSolution* eegfwd = NULL;
    if (read_one(t_pStream, eegnode, eegfwd))
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
        if (megfwd->sol->data->cols() != eegfwd->sol->data->cols() ||
                megfwd->source_ori != eegfwd->source_ori ||
                megfwd->nsource != eegfwd->nsource ||
                megfwd->coord_frame != eegfwd->coord_frame)
        {
            t_pStream->device()->close();
            std::cout << "The MEG and EEG forward solutions do not match\n"; // ToDo throw error
            //garbage collecting
            if(t_pDir)
                delete t_pDir;
            if(t_pTree)
                delete t_pTree;
            if(t_pStream)
                delete t_pStream;
            if(t_pSourceSpace)
                delete t_pSourceSpace;
            return false;
        }

        fwd = new MNEForwardSolution(megfwd);
        fwd->sol->data = new MatrixXd(megfwd->sol->nrow + eegfwd->sol->nrow, megfwd->sol->ncol);

        fwd->sol->data->block(0,0,megfwd->sol->nrow,megfwd->sol->ncol) = *megfwd->sol->data;
        fwd->sol->data->block(megfwd->sol->nrow,0,eegfwd->sol->nrow,eegfwd->sol->ncol) = *eegfwd->sol->data;
        fwd->sol->nrow = megfwd->sol->nrow + eegfwd->sol->nrow;
        fwd->sol->row_names.append(eegfwd->sol->row_names);

        if (fwd->sol_grad)
        {
            fwd->sol_grad->data->resize(megfwd->sol_grad->data->rows() + eegfwd->sol_grad->data->rows(), megfwd->sol_grad->data->cols());

            fwd->sol->data->block(0,0,megfwd->sol_grad->data->rows(),megfwd->sol_grad->data->cols()) = *megfwd->sol_grad->data;
            fwd->sol->data->block(megfwd->sol_grad->data->rows(),0,eegfwd->sol_grad->data->rows(),eegfwd->sol_grad->data->cols()) = *eegfwd->sol_grad->data;

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
    if(!parent_mri.at(0)->find_tag(t_pStream, FIFF_COORD_TRANS, t_pTag))
    {
        t_pStream->device()->close();
        std::cout << "MRI/head coordinate transformation not found\n"; // ToDo throw error
        //garbage collecting
        if(t_pDir)
            delete t_pDir;
        if(t_pTree)
            delete t_pTree;
        if(t_pStream)
            delete t_pStream;
        if(t_pSourceSpace)
            delete t_pSourceSpace;
        if(t_pTag)
            delete t_pTag;
        return false;
    }
    else
    {
        if(fwd->mri_head_t)
            delete fwd->mri_head_t;
        fwd->mri_head_t = t_pTag->toCoordTrans();
//            std::cout << "Transformation\n" << fwd->mri_head_t.trans << std::endl;
//            std::cout << "from " << fwd->mri_head_t.from << " to " << fwd->mri_head_t.to << std::endl;

//            std::cout << "inverse Transformation\n" << fwd->mri_head_t.invtrans << std::endl;
//            std::cout << "size " << sizeof(FiffCoordTrans) << std::endl;
//            std::cout << "size " << sizeof(fiffCoordTransRec) << std::endl;

        if (fwd->mri_head_t->from != FIFFV_COORD_MRI || fwd->mri_head_t->to != FIFFV_COORD_HEAD)
        {
            fwd->mri_head_t->invert_transform();
            if (fwd->mri_head_t->from != FIFFV_COORD_MRI || fwd->mri_head_t->to != FIFFV_COORD_HEAD)
            {
                t_pStream->device()->close();
                std::cout << "MRI/head coordinate transformation not found\n"; // ToDo throw error
                //garbage collecting
                if(t_pDir)
                    delete t_pDir;
                if(t_pTree)
                    delete t_pTree;
                if(t_pStream)
                    delete t_pStream;
                if(t_pSourceSpace)
                    delete t_pSourceSpace;
                if(t_pTag)
                    delete t_pTag;
                return false;
            }
        }
    }
    t_pStream->device()->close();

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
    t_pSourceSpace->transform_source_space_to(fwd->coord_frame,fwd->mri_head_t);
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
        fwd->source_rr = MatrixXd::Zero(fwd->nsource,3);
        fwd->source_nn = MatrixXd::Zero(fwd->nsource,3);
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

            MatrixXd tmp = fwd->source_nn.transpose();
            SparseMatrix<double>* fix_rot = MNEMath::make_block_diag(&tmp,1);
            *fwd->sol->data  = (*fwd->sol->data)*(*fix_rot);
            fwd->sol->ncol  = fwd->nsource;
            fwd->source_ori = FIFFV_MNE_FIXED_ORI;

            if (fwd->sol_grad != NULL)
            {
                SparseMatrix<double> t_matKron;
                SparseMatrix<double> t_eye(3,3);
                for (qint32 i = 0; i < 3; ++i)
                    t_eye.insert(i,i) = 1.0f;
                kroneckerProduct(*fix_rot,t_eye,t_matKron);//kron(fix_rot,eye(3));
                *fwd->sol_grad->data   = (*fwd->sol_grad->data)*t_matKron;
                fwd->sol_grad->ncol   = 3*fwd->nsource;
            }
            delete fix_rot;
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
            fwd->source_rr = MatrixXd::Zero(fwd->nsource,3);
            fwd->source_nn = MatrixXd::Zero(fwd->nsource*3,3);

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
                    Vector3d nn = t_pSourceSpace->hemispheres.at(k)->nn.block(t_pSourceSpace->hemispheres.at(k)->vertno(p),0,1,3).transpose();

                    Matrix3d tmp = Matrix3d::Identity() - nn*nn.transpose();

                    JacobiSVD<MatrixXd> t_svd(tmp, Eigen::ComputeThinU);

                    MatrixXd U(t_svd.matrixU());

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
            MatrixXd tmp = fwd->source_nn.transpose();
            SparseMatrix<double>* surf_rot = MNEMath::make_block_diag(&tmp,3);

            *fwd->sol->data = (*fwd->sol->data)*(*surf_rot);

            if (fwd->sol_grad != NULL)
            {
                SparseMatrix<double> t_matKron;
                SparseMatrix<double> t_eye(3,3);
                for (qint32 i = 0; i < 3; ++i)
                    t_eye.insert(i,i) = 1.0f;
                kroneckerProduct(*surf_rot,t_eye,t_matKron);//kron(surf_rot,eye(3));
                (*fwd->sol_grad->data)   = (*fwd->sol_grad->data)*t_matKron;
            }
            delete surf_rot;
            printf("[done]\n");
        }
        else
        {
            printf("\tCartesian source orientations...");
            nuse = 0;
            fwd->source_rr = MatrixXd::Zero(fwd->nsource,3);
            for(int k = 0; k < t_pSourceSpace->hemispheres.size(); ++k)
            {
                for (qint32 q = 0; q < t_pSourceSpace->hemispheres.at(k)->nuse; ++q)
                    fwd->source_rr.block(q+nuse,0,1,3) = t_pSourceSpace->hemispheres.at(k)->rr.block(t_pSourceSpace->hemispheres.at(k)->vertno(q),0,1,3);

                nuse += t_pSourceSpace->hemispheres.at(k)->nuse;
            }

            MatrixXd t_ones = MatrixXd::Ones(fwd->nsource,1);
            Matrix3d t_eye = Matrix3d::Identity();
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
        MatrixXd* t_solData = new MatrixXd(nuse,fwd->sol->data->cols());
        QStringList t_solRowNames;

        MatrixXd* t_solGradData = NULL;
        QStringList t_solGradRowNames;

        if (fwd->sol_grad != NULL)
            t_solGradData = new MatrixXd(nuse, fwd->sol_grad->data->cols());

        for(qint32 k = 0; k < fwd->nchan; ++k)
        {
            if(pick(k) > 0)
            {
                t_solData->block(p, 0, 1, fwd->sol->data->cols()) = fwd->sol->data->block(k, 0, 1, fwd->sol->data->cols());
                t_solRowNames.append(fwd->sol->row_names.at(k));

                if (fwd->sol_grad != NULL)
                {
                    t_solGradData->block(p, 0, 1, fwd->sol_grad->data->cols()) = fwd->sol_grad->data->block(k, 0, 1, fwd->sol_grad->data->cols());
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
        if(fwd->sol->data)
            delete fwd->sol->data;
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
    t_pStream->device()->close();

    //garbage collecting
//    if (megfwd)
//        delete megfwd; // don't delete the megfwd because fwd->src is pointing to source space (not copied for the sake of speed)
//    if (eegfwd)
//        delete eegfwd; // don't delete the eegfwd because fwd->src is pointing to source space (not copied for the sake of speed)
    if(t_pDir)
        delete t_pDir;
    if(t_pTree)
        delete t_pTree;
    if(t_pStream)
        delete t_pStream;
//    if(t_pSourceSpace)
//        delete t_pSourceSpace; // don't delete the SourceSpace because fwd->src is pointing to source space (not copied for the sake of speed)
    if(t_pTag)
        delete t_pTag;

    return true;
}


//*************************************************************************************************************

bool MNEForwardSolution::read_one(FiffStream* p_pStream, FiffDirTree* node, MNEForwardSolution*& one)
{
    if (one != NULL)
        delete one;
    one = NULL;
    //
    //   Read all interesting stuff for one forward solution
    //
    if(node == NULL)
    {
        return false;
    }

    one = new MNEForwardSolution();
    FIFFLIB::FiffTag* t_pTag = NULL;

    if(!node->find_tag(p_pStream, FIFF_MNE_SOURCE_ORIENTATION, t_pTag))
    {
        p_pStream->device()->close();
        std::cout << "Source orientation tag not found."; //ToDo: throw error.
        return false;
    }

    one->source_ori = *t_pTag->toInt();

    if(!node->find_tag(p_pStream, FIFF_MNE_COORD_FRAME, t_pTag))
    {
        p_pStream->device()->close();
        std::cout << "Coordinate frame tag not found."; //ToDo: throw error.
        return false;
    }

    one->coord_frame = *t_pTag->toInt();

    if(!node->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag))
    {
        p_pStream->device()->close();
        std::cout << "Number of sources not found."; //ToDo: throw error.
        return false;
    }

    one->nsource = *t_pTag->toInt();

    if(!node->find_tag(p_pStream, FIFF_NCHAN, t_pTag))
    {
        p_pStream->device()->close();
        printf("Number of channels not found."); //ToDo: throw error.
        return false;
    }

    one->nchan = *t_pTag->toInt();

    if(p_pStream->read_named_matrix(node, FIFF_MNE_FORWARD_SOLUTION, one->sol))
        one->sol->transpose_named_matrix();
    else
    {
        p_pStream->device()->close();
        printf("Forward solution data not found ."); //ToDo: throw error.
        //error(me,'Forward solution data not found (%s)',mne_omit_first_line(lasterr));
        return false;
    }

    if(p_pStream->read_named_matrix(node, FIFF_MNE_FORWARD_SOLUTION_GRAD, one->sol_grad))
        one->sol_grad->transpose_named_matrix();
    else
    {
        if (one->sol_grad)
            delete one->sol_grad;
        one->sol_grad = NULL;
    }


    if (one->sol->data->rows() != one->nchan ||
            (one->sol->data->cols() != one->nsource && one->sol->data->cols() != 3*one->nsource))
    {
        p_pStream->device()->close();
        printf("Forward solution matrix has wrong dimensions.\n"); //ToDo: throw error.
        //error(me,'Forward solution matrix has wrong dimensions');
        return false;
    }
    if (one->sol_grad)
    {
        if (one->sol_grad->data->rows() != one->nchan ||
                (one->sol_grad->data->cols() != 3*one->nsource && one->sol_grad->data->cols() != 3*3*one->nsource))
        {
            p_pStream->device()->close();
            printf("Forward solution gradient matrix has wrong dimensions.\n"); //ToDo: throw error.
            //error(me,'Forward solution gradient matrix has wrong dimensions');
        }
    }
    if (t_pTag)
        delete t_pTag;
    return true;
}
