//=============================================================================================================
/**
 * @file     mne_sourcespace.cpp
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
 * @brief    MNESourceSpace class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_sourcespace.h"

#include <utils/mnemath.h>
#include <fs/label.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FSLIB;
using namespace MNELIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESourceSpace::MNESourceSpace()
{
}

//=============================================================================================================

MNESourceSpace::MNESourceSpace(const MNESourceSpace &p_MNESourceSpace)
: m_qListHemispheres(p_MNESourceSpace.m_qListHemispheres)
{
}

//=============================================================================================================

MNESourceSpace::~MNESourceSpace()
{
}

//=============================================================================================================

void MNESourceSpace::clear()
{
    m_qListHemispheres.clear();
}

//=============================================================================================================

QList<VectorXi> MNESourceSpace::get_vertno() const
{
    QList<VectorXi> p_vertices;
    for(qint32 i = 0; i < m_qListHemispheres.size(); ++i)
        p_vertices.push_back(m_qListHemispheres[i].vertno);
    return p_vertices;
}

//=============================================================================================================

QList<VectorXi> MNESourceSpace::label_src_vertno_sel(const Label &p_label, VectorXi &src_sel) const
{
//    if(src[0].['type'] != 'surf')
//        return Exception('Label are only supported with surface source spaces')

    QList<VectorXi> vertno;
    vertno << this->m_qListHemispheres[0].vertno << this->m_qListHemispheres[1].vertno;

    if (p_label.hemi == 0) //lh
    {
        VectorXi vertno_sel = MNEMath::intersect(vertno[0], p_label.vertices, src_sel);
        vertno[0] = vertno_sel;
        vertno[1] = VectorXi();
    }
    else if (p_label.hemi == 1) //rh
    {
        VectorXi vertno_sel = MNEMath::intersect(vertno[1], p_label.vertices, src_sel);
        src_sel.array() += p_label.vertices.size();
        vertno[0] = VectorXi();
        vertno[1] = vertno_sel;
    }

//    if (p_label.hemi == 0) //lh
//    {
//        VectorXi vertno_sel = MNEMath::intersect(vertno[0], p_label.vertices[0], src_sel);
//        vertno[0] = vertno_sel;
//        vertno[1] = VectorXi();
//    }
//    else if (p_label.hemi == 1) //rh
//    {
//        VectorXi vertno_sel = MNEMath::intersect(vertno[1], p_label.vertices[1], src_sel);
//        src_sel.array() += p_label.vertices[0].size();
//        vertno[0] = VectorXi();
//        vertno[1] = vertno_sel;
//    }
//    else if (p_label.hemi == 2) //both
//    {
//        VectorXi src_sel_lh, src_sel_rh;
//        VectorXi vertno_sel_lh = MNEMath::intersect(vertno[0], p_label.vertices[0], src_sel_lh);
//        VectorXi vertno_sel_rh = MNEMath::intersect(vertno[1], p_label.vertices[1], src_sel_rh);
//        src_sel.resize(src_sel_lh.size() + src_sel_rh.size());
//        src_sel.block(0,0,src_sel_lh.size(),1) = src_sel_lh;
//        src_sel.block(src_sel_lh.size(),0,src_sel_rh.size(),1) = src_sel_rh;
//        vertno[0] = vertno_sel_lh;
//        vertno[0] = vertno_sel_rh;
//    }
    else
    {
        qWarning("Unknown hemisphere type\n");
        vertno[0] = VectorXi::Zero(0);
        vertno[1] = VectorXi::Zero(0);
    }

    return vertno;
}

//=============================================================================================================

MNESourceSpace MNESourceSpace::pick_regions(const QList<Label> &p_qListLabels) const
{
    Q_UNUSED(p_qListLabels);

    MNESourceSpace selectedSrc(*this);

    for(qint32 h = 0; h < 2; ++h)
    {
        VectorXi selVertices;

        //get vertices indeces for new selection
        qint32 iSize = 0;
        for(qint32 i = 0; i < p_qListLabels.size(); ++i)
        {
            if(p_qListLabels[i].hemi == h)
            {
                VectorXi currentSelection;

                MNEMath::intersect(m_qListHemispheres[h].vertno, p_qListLabels[i].vertices, currentSelection);

                selVertices.conservativeResize(iSize+currentSelection.size());
                selVertices.block(iSize,0,currentSelection.size(),1) = currentSelection;
                iSize = selVertices.size();
            }
        }

        MNEMath::sort(selVertices, false);

        VectorXi newVertno(selVertices.size());

        selectedSrc.m_qListHemispheres[h].inuse = VectorXi::Zero(selectedSrc.m_qListHemispheres[h].np);

        for(qint32 i = 0; i < selVertices.size(); ++i)
        {
            selectedSrc.m_qListHemispheres[h].inuse[selVertices[i]] = 1;
            newVertno[i] = this->m_qListHemispheres[h].vertno[selVertices[i]];
        }

        selectedSrc.m_qListHemispheres[h].nuse = selVertices.size();
        selectedSrc.m_qListHemispheres[h].vertno = newVertno;

        //
        // Tris
        //
        VectorXi idx_select = VectorXi::Zero(this->m_qListHemispheres[h].use_tris.rows());
        for(qint32 i = 0; i < 3; ++i)
        {
            VectorXi tri_dim = this->m_qListHemispheres[h].use_tris.col(i);
            VectorXi idx_dim;
            MNEMath::intersect(tri_dim, newVertno, idx_dim);

            for(qint32 j = 0; j < idx_dim.size(); ++j)
                idx_select[idx_dim[j]] = 1;
        }

        qint32 countSel = 0;
        for(qint32 i = 0; i < idx_select.size(); ++i)
            if(idx_select[i] == 1)
                ++countSel;

        selectedSrc.m_qListHemispheres[h].nuse_tri = countSel;

        MatrixX3i use_tris_new(countSel,3);
        MatrixX3d use_tri_cent_new(countSel,3);
        MatrixX3d use_tri_nn_new(countSel,3);
        VectorXd use_tri_area_new(countSel);

        countSel = 0;
        for(qint32 i = 0; i < idx_select.size(); ++i)
        {
            if(idx_select[i] == 1)
            {
                use_tris_new.row(countSel) = this->m_qListHemispheres[h].use_tris.row(i);
                use_tri_cent_new.row(countSel) = this->m_qListHemispheres[h].use_tri_cent.row(i);
                use_tri_nn_new.row(countSel) = this->m_qListHemispheres[h].use_tri_nn.row(i);
                use_tri_area_new[countSel] = this->m_qListHemispheres[h].use_tri_area[i];
                ++countSel;
            }
        }

        selectedSrc.m_qListHemispheres[h].use_tris = use_tris_new;
        selectedSrc.m_qListHemispheres[h].use_tri_cent = use_tri_cent_new;
        selectedSrc.m_qListHemispheres[h].use_tri_nn = use_tri_nn_new;
        selectedSrc.m_qListHemispheres[h].use_tri_area = use_tri_area_new;
    }

    return selectedSrc;
}

//=============================================================================================================

bool MNESourceSpace::readFromStream(FiffStream::SPtr& p_pStream,
                                    bool add_geom,
                                    MNESourceSpace& p_SourceSpace)
{
//    if (p_pSourceSpace != NULL)
//        delete p_pSourceSpace;
    p_SourceSpace = MNESourceSpace();

    //
    //   Open the file, create directory
    //
    bool open_here = false;
    QFile t_file;//ToDo TCPSocket;

    if (!p_pStream->device()->isOpen())
    {
        QString t_sFileName = p_pStream->streamName();

        t_file.setFileName(t_sFileName);
        p_pStream = FiffStream::SPtr(new FiffStream(&t_file));
        if(!p_pStream->open())
            return false;
        open_here = true;
//        if(t_pDir)
//            delete t_pDir;
    }
    //
    //   Find all source spaces
    //
    QList<FiffDirNode::SPtr> spaces = p_pStream->dirtree()->dir_tree_find(FIFFB_MNE_SOURCE_SPACE);
    if (spaces.size() == 0)
    {
        if(open_here)
            p_pStream->close();
        std::cout << "No source spaces found";
        return false;
    }

    for(int k = 0; k < spaces.size(); ++k)
    {
        MNEHemisphere p_Hemisphere;
        printf("\tReading a source space...");
        MNESourceSpace::read_source_space(p_pStream, spaces[k], p_Hemisphere);
        printf("\t[done]\n" );
        if (add_geom)
            complete_source_space_info(p_Hemisphere);

        p_SourceSpace.m_qListHemispheres.append(p_Hemisphere);

//           src(k) = this;
    }

    printf("\t%d source spaces read\n", spaces.size());

    if(open_here)
        p_pStream->close();

    return true;
}

//=============================================================================================================

qint32 MNESourceSpace::find_source_space_hemi(MNEHemisphere& p_Hemisphere)
{
    double xave = p_Hemisphere.rr.col(0).sum();

    qint32 hemi;
    if (xave < 0)
        hemi = FIFFV_MNE_SURF_LEFT_HEMI;
    else
        hemi = FIFFV_MNE_SURF_RIGHT_HEMI;

    return hemi;
}

//=============================================================================================================

bool MNESourceSpace::transform_source_space_to(fiff_int_t dest, FiffCoordTrans& trans)
{
    for(int k = 0; k < this->m_qListHemispheres.size(); ++k)
    {
        if(!this->m_qListHemispheres[k].transform_hemisphere_to(dest,trans))
        {
            printf("Could not transform source space.\n");
            return false;
        }
    }
    return true;
}

//=============================================================================================================

bool MNESourceSpace::read_source_space(FiffStream::SPtr& p_pStream, const FiffDirNode::SPtr& p_Tree, MNEHemisphere& p_Hemisphere)
{
    p_Hemisphere.clear();

    FiffTag::SPtr t_pTag;

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_ID, t_pTag))
        p_Hemisphere.id = FIFFV_MNE_SURF_UNKNOWN;
    else
        p_Hemisphere.id = *t_pTag->toInt();

//        qDebug() << "Read SourceSpace ID; type:" << t_pTag->getType() << "value:" << *t_pTag->toInt();

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag))
    {
        p_pStream->close();
        std::cout << "error: Number of vertices not found."; //ToDo: throw error.
        return false;
    }
//        qDebug() << "Number of vertice; type:" << t_pTag->getType() << "value:" << *t_pTag->toInt();
    p_Hemisphere.np = *t_pTag->toInt();

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_BEM_SURF_NTRI, t_pTag))
    {
        if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NTRI, t_pTag))
            p_Hemisphere.ntri = 0;
        else
            p_Hemisphere.ntri = *t_pTag->toInt();
    }
    else
    {
        p_Hemisphere.ntri = *t_pTag->toInt();
    }
//        qDebug() << "Number of Tris; type:" << t_pTag->getType() << "value:" << *t_pTag->toInt();

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_COORD_FRAME, t_pTag))
    {
        p_pStream->close();
        std::cout << "Coordinate frame information not found."; //ToDo: throw error.
        return false;
    }
    p_Hemisphere.coord_frame = *t_pTag->toInt();
//        qDebug() << "Coord Frame; type:" << t_pTag->getType() << "value:" << *t_pTag->toInt();

    //=====================================================================
    //
    //   Vertices, normals, and triangles
    //
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_POINTS, t_pTag))
    {
        p_pStream->close();
        std::cout << "Vertex data not found."; //ToDo: throw error.
        return false;
    }

    p_Hemisphere.rr = t_pTag->toFloatMatrix().transpose();
    qint32 rows_rr = p_Hemisphere.rr.rows();
//        qDebug() << "last element rr: " << p_Hemisphere.rr(rows_rr-1, 0) << p_Hemisphere.rr(rows_rr-1, 1) << p_Hemisphere.rr(rows_rr-1, 2);

    if (rows_rr != p_Hemisphere.np)
    {
        p_pStream->close();
        std::cout << "Vertex information is incorrect."; //ToDo: throw error.
        return false;
    }
//        qDebug() << "Source Space Points; type:" << t_pTag->getType();

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NORMALS, t_pTag))
    {
        p_pStream->close();
        std::cout << "Vertex normals not found."; //ToDo: throw error.
        return false;
    }

    p_Hemisphere.nn = t_pTag->toFloatMatrix().transpose();
    qint32 rows_nn = p_Hemisphere.nn.rows();

    if (rows_nn != p_Hemisphere.np)
    {
        p_pStream->close();
        std::cout << "Vertex normal information is incorrect."; //ToDo: throw error.
        return false;
    }
//        qDebug() << "Source Space Normals; type:" << t_pTag->getType();

    //=====================================================================
    if (p_Hemisphere.ntri > 0)
    {
        if(!p_Tree->find_tag(p_pStream, FIFF_BEM_SURF_TRIANGLES, t_pTag))
        {
            if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_TRIANGLES, t_pTag))
            {
                p_pStream->close();
                std::cout << "Triangulation not found."; //ToDo: throw error.
                return false;
            }
            else
            {
                p_Hemisphere.tris = t_pTag->toIntMatrix().transpose();
                p_Hemisphere.tris -= MatrixXi::Constant(p_Hemisphere.tris.rows(),3,1);//0 based indizes
            }
        }
        else
        {
            p_Hemisphere.tris = t_pTag->toIntMatrix().transpose();
            p_Hemisphere.tris -= MatrixXi::Constant(p_Hemisphere.tris.rows(),3,1);//0 based indizes
        }
        if (p_Hemisphere.tris.rows() != p_Hemisphere.ntri)
        {
            p_pStream->close();
            std::cout << "Triangulation information is incorrect."; //ToDo: throw error.
            return false;
        }
    }
    else
    {
        MatrixXi p_defaultMatrix(0, 0);
        p_Hemisphere.tris = p_defaultMatrix;
    }
//        qDebug() << "Triangles; type:" << t_pTag->getType() << "rows:" << p_Hemisphere.tris.rows() << "cols:" << p_Hemisphere.tris.cols();

    //
    //   Which vertices are active
    //
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NUSE, t_pTag))
    {
        p_Hemisphere.nuse   = 0;
        p_Hemisphere.inuse  = VectorXi::Zero(p_Hemisphere.nuse);
        VectorXi p_defaultVector;
        p_Hemisphere.vertno = p_defaultVector;
    }
    else
    {
        p_Hemisphere.nuse = *t_pTag->toInt();
        if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_SELECTION, t_pTag))
        {
            p_pStream->close();
            std::cout << "Source selection information missing."; //ToDo: throw error.
            return false;
        }
        p_Hemisphere.inuse = VectorXi(Map<VectorXi>(t_pTag->toInt(), t_pTag->size()/4, 1));//use copy constructor, for the sake of easy memory management

        p_Hemisphere.vertno = VectorXi::Zero(p_Hemisphere.nuse);
        if (p_Hemisphere.inuse.rows() != p_Hemisphere.np)
        {
            p_pStream->close();
            std::cout << "Incorrect number of entries in source space selection."; //ToDo: throw error.
            return false;
        }
        int pp = 0;
        for (int p = 0; p < p_Hemisphere.np; ++p)
        {
            if(p_Hemisphere.inuse(p) == 1)
            {
                p_Hemisphere.vertno(pp) = p;
                ++pp;
            }
        }
    }
//        qDebug() << "Vertices; type:" << t_pTag->getType() << "nuse:" << p_Hemisphere.nuse;

    //
    //   Use triangulation
    //
    FiffTag::SPtr t_pTag1;
    FiffTag::SPtr t_pTag2;
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NUSE_TRI, t_pTag1) || !p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, t_pTag2))
    {
        MatrixX3i p_defaultMatrix;
        p_Hemisphere.nuse_tri = 0;
        p_Hemisphere.use_tris = p_defaultMatrix;
    }
    else
    {
        p_Hemisphere.nuse_tri = *t_pTag1->toInt();
        p_Hemisphere.use_tris = t_pTag2->toIntMatrix().transpose();
        p_Hemisphere.use_tris -= MatrixXi::Constant(p_Hemisphere.use_tris.rows(),3,1); //0 based indizes
    }
//        qDebug() << "triangulation; type:" << t_pTag2->getType() << "use_tris:" << p_Hemisphere.use_tris.rows()<< "x" << p_Hemisphere.use_tris.cols();

    //
    //   Patch-related information
    //
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NEAREST, t_pTag1) || !p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, t_pTag2))
    {
        VectorXi p_defaultVector;
        p_Hemisphere.nearest = p_defaultVector;
        VectorXd p_defaultFloatVector;
        p_Hemisphere.nearest_dist = p_defaultFloatVector;
    }
    else
    {
       //res.nearest = tag1.data + 1;
       p_Hemisphere.nearest = VectorXi(Map<VectorXi>(t_pTag1->toInt(), t_pTag1->size()/4, 1));//use copy constructor, for the sake of easy memory management
       p_Hemisphere.nearest_dist = VectorXd((Map<VectorXf>(t_pTag2->toFloat(), t_pTag1->size()/4, 1)).cast<double>());//use copy constructor, for the sake of easy memory management
    }

//    patch_info(p_Hemisphere.nearest, p_Hemisphere.pinfo);
    if (patch_info(p_Hemisphere))
       printf("\tPatch information added...");

    //
    // Distances
    //
//    if(p_Hemisphere.dist)
//        delete p_Hemisphere.dist;
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_DIST, t_pTag1) || !p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, t_pTag2))
    {
       p_Hemisphere.dist = SparseMatrix<double>();//NULL;
       p_Hemisphere.dist_limit = 0;
    }
    else
    {
        p_Hemisphere.dist       = t_pTag1->toSparseFloatMatrix();
        p_Hemisphere.dist_limit = *t_pTag2->toFloat(); //ToDo Check if this is realy always a float and not a matrix
        //
        //  Add the upper triangle
        //
        SparseMatrix<double> distT = p_Hemisphere.dist.transpose();
        p_Hemisphere.dist += distT;
    }

    return true;
}

//=============================================================================================================

bool MNESourceSpace::patch_info(MNEHemisphere &p_Hemisphere)//VectorXi& nearest, QList<VectorXi>& pinfo)
{
    if (p_Hemisphere.nearest.rows() == 0)
    {
       p_Hemisphere.pinfo.clear();
       p_Hemisphere.patch_inds = VectorXi();
       return false;
    }

    printf("\tComputing patch statistics...");

    std::vector< std::pair<int,int> > t_vIndn;

    for(qint32 i = 0; i < p_Hemisphere.nearest.rows(); ++i)
    {
        std::pair<int,int> t_pair(i, p_Hemisphere.nearest(i));
        t_vIndn.push_back(t_pair);
    }
    std::sort(t_vIndn.begin(),t_vIndn.end(), MNEMath::compareIdxValuePairSmallerThan<int> );

    VectorXi nearest_sorted(t_vIndn.size());

    qint32 current = 0;
    std::vector<qint32> t_vfirsti;
    t_vfirsti.push_back(current);
    std::vector<qint32> t_vlasti;

    for(quint32 i = 0; i < t_vIndn.size(); ++i)
    {
        nearest_sorted[i] = t_vIndn[i].second;
        if (t_vIndn[current].second != t_vIndn[i].second)
        {
            current = i;
            t_vlasti.push_back(i-1);
            t_vfirsti.push_back(current);
        }
    }
    t_vlasti.push_back(static_cast<int>(t_vIndn.size()-1));

    for(quint32 k = 0; k < t_vfirsti.size(); ++k)
    {
        std::vector<int> t_vIndex;

        for(int l = t_vfirsti[k]; l <= t_vlasti[k]; ++l)
            t_vIndex.push_back(t_vIndn[l].first);

        std::sort(t_vIndex.begin(),t_vIndex.end());

        int* t_pV = &t_vIndex[0];
        Eigen::Map<Eigen::VectorXi> t_vPInfo(t_pV, t_vIndex.size());

        p_Hemisphere.pinfo.append(t_vPInfo);
    }

    // compute patch indices of the in-use source space vertices
    std::vector<qint32> patch_verts;
    patch_verts.reserve(t_vlasti.size());
    for(quint32 i = 0; i < t_vlasti.size(); ++i)
        patch_verts.push_back(nearest_sorted[t_vlasti[i]]);

    p_Hemisphere.patch_inds.resize(p_Hemisphere.vertno.size());
    std::vector<qint32>::iterator it;
    for(qint32 i = 0; i < p_Hemisphere.vertno.size(); ++i)
    {
        it = std::find(patch_verts.begin(), patch_verts.end(), p_Hemisphere.vertno[i]);
        p_Hemisphere.patch_inds[i] = it-patch_verts.begin();
    }

    return true;
}

//=============================================================================================================

bool MNESourceSpace::complete_source_space_info(MNEHemisphere& p_Hemisphere)
{
    //
    //   Main triangulation
    //
    printf("\tCompleting triangulation info...");
    p_Hemisphere.tri_cent = MatrixX3d::Zero(p_Hemisphere.ntri,3);
    p_Hemisphere.tri_nn = MatrixX3d::Zero(p_Hemisphere.ntri,3);
    p_Hemisphere.tri_area = VectorXd::Zero(p_Hemisphere.ntri);

    Matrix3d r;
    Vector3d a, b;
    int k = 0;
    float size = 0;
    for (qint32 i = 0; i < p_Hemisphere.ntri; ++i)
    {
        for ( qint32 j = 0; j < 3; ++j)
        {
            k = p_Hemisphere.tris(i, j);

            r(j,0) = p_Hemisphere.rr(k, 0);
            r(j,1) = p_Hemisphere.rr(k, 1);
            r(j,2) = p_Hemisphere.rr(k, 2);

            p_Hemisphere.tri_cent(i, 0) += p_Hemisphere.rr(k, 0);
            p_Hemisphere.tri_cent(i, 1) += p_Hemisphere.rr(k, 1);
            p_Hemisphere.tri_cent(i, 2) += p_Hemisphere.rr(k, 2);
        }
        p_Hemisphere.tri_cent.row(i) /= 3.0f;

        //cross product {cross((r2-r1),(r3-r1))}
        a = r.row(1) - r.row(0 );
        b = r.row(2) - r.row(0);
        p_Hemisphere.tri_nn(i,0) = a(1)*b(2)-a(2)*b(1);
        p_Hemisphere.tri_nn(i,1) = a(2)*b(0)-a(0)*b(2);
        p_Hemisphere.tri_nn(i,2) = a(0)*b(1)-a(1)*b(0);

        //area
        size = p_Hemisphere.tri_nn.row(i)*p_Hemisphere.tri_nn.row(i).transpose();
        size = std::pow(size, 0.5f );

        p_Hemisphere.tri_area(i) = size/2.0f;
        p_Hemisphere.tri_nn.row(i) /= size;

    }
    printf("[done]\n");

//        qDebug() << "p_Hemisphere.tri_cent:" << p_Hemisphere.tri_cent(0,0) << p_Hemisphere.tri_cent(0,1) << p_Hemisphere.tri_cent(0,2);
//        qDebug() << "p_Hemisphere.tri_cent:" << p_Hemisphere.tri_cent(2,0) << p_Hemisphere.tri_cent(2,1) << p_Hemisphere.tri_cent(2,2);

//        qDebug() << "p_Hemisphere.tri_nn:" << p_Hemisphere.tri_nn(0,0) << p_Hemisphere.tri_nn(0,1) << p_Hemisphere.tri_nn(0,2);
//        qDebug() << "p_Hemisphere.tri_nn:" << p_Hemisphere.tri_nn(2,0) << p_Hemisphere.tri_nn(2,1) << p_Hemisphere.tri_nn(2,2);

    //
    //   Selected triangles
    //
    printf("\tCompleting selection triangulation info...");
    if (p_Hemisphere.nuse_tri > 0)
    {
        p_Hemisphere.use_tri_cent = MatrixX3d::Zero(p_Hemisphere.nuse_tri,3);
        p_Hemisphere.use_tri_nn = MatrixX3d::Zero(p_Hemisphere.nuse_tri,3);
        p_Hemisphere.use_tri_area = VectorXd::Zero(p_Hemisphere.nuse_tri);

        for (qint32 i = 0; i < p_Hemisphere.nuse_tri; ++i)
        {
            for ( qint32 j = 0; j < 3; ++j)
            {
                k = p_Hemisphere.use_tris(i, j);

                r(j,0) = p_Hemisphere.rr(k, 0);
                r(j,1) = p_Hemisphere.rr(k, 1);
                r(j,2) = p_Hemisphere.rr(k, 2);

                p_Hemisphere.use_tri_cent(i, 0) += p_Hemisphere.rr(k, 0);
                p_Hemisphere.use_tri_cent(i, 1) += p_Hemisphere.rr(k, 1);
                p_Hemisphere.use_tri_cent(i, 2) += p_Hemisphere.rr(k, 2);
            }
            p_Hemisphere.use_tri_cent.row(i) /= 3.0f;

            //cross product {cross((r2-r1),(r3-r1))}
            a = r.row(1) - r.row(0 );
            b = r.row(2) - r.row(0);
            p_Hemisphere.use_tri_nn(i,0) = a(1)*b(2)-a(2)*b(1);
            p_Hemisphere.use_tri_nn(i,1) = a(2)*b(0)-a(0)*b(2);
            p_Hemisphere.use_tri_nn(i,2) = a(0)*b(1)-a(1)*b(0);

            //area
            size = p_Hemisphere.use_tri_nn.row(i)*p_Hemisphere.use_tri_nn.row(i).transpose();
            size = std::pow(size, 0.5f );

            p_Hemisphere.use_tri_area(i) = size/2.0f;
        }

    }
    printf("[done]\n");

//        qDebug() << "p_Hemisphere.use_tri_cent:" << p_Hemisphere.use_tri_cent(0,0) << p_Hemisphere.use_tri_cent(0,1) << p_Hemisphere.use_tri_cent(0,2);
//        qDebug() << "p_Hemisphere.use_tri_cent:" << p_Hemisphere.use_tri_cent(2,0) << p_Hemisphere.use_tri_cent(2,1) << p_Hemisphere.use_tri_cent(2,2);

//        qDebug() << "p_Hemisphere.use_tri_nn:" << p_Hemisphere.use_tri_nn(0,0) << p_Hemisphere.use_tri_nn(0,1) << p_Hemisphere.use_tri_nn(0,2);
//        qDebug() << "p_Hemisphere.use_tri_nn:" << p_Hemisphere.use_tri_nn(2,0) << p_Hemisphere.use_tri_nn(2,1) << p_Hemisphere.use_tri_nn(2,2);

    printf("\tCompleting triangle and vertex neighboring info...");
    p_Hemisphere.add_geometry_info();
    printf("[done]\n");

    return true;
}

//=============================================================================================================

void MNESourceSpace::writeToStream(FiffStream* p_pStream)
{
    for(qint32 h = 0; h < m_qListHemispheres.size(); ++h)
    {
        printf("\tWrite a source space... ");
        p_pStream->start_block(FIFFB_MNE_SOURCE_SPACE);
        m_qListHemispheres[h].writeToStream(p_pStream);
        p_pStream->end_block(FIFFB_MNE_SOURCE_SPACE);
        printf("[done]\n");
    }
    printf("\t%d source spaces written\n", m_qListHemispheres.size());
}

//=============================================================================================================

MNEHemisphere& MNESourceSpace::operator[] (qint32 idx)
{
    if(m_qListHemispheres.size() > idx)
        return m_qListHemispheres[idx];
    else
    {
        qWarning("Warning: Index out of bound! Returning last element.");
        return m_qListHemispheres[m_qListHemispheres.size()-1];
    }
}

//=============================================================================================================

const MNEHemisphere& MNESourceSpace::operator[] (qint32 idx) const
{
    if(m_qListHemispheres.size() > idx)
        return m_qListHemispheres[idx];
    else
    {
        qWarning("Warning: Index out of bound! Returning last element.");
        return m_qListHemispheres[m_qListHemispheres.size()-1];
    }
}

//=============================================================================================================

MNEHemisphere& MNESourceSpace::operator[] (QString idt)
{
    if(idt.compare("lh") == 0)
        return m_qListHemispheres[0];
    else if(idt.compare("rh") == 0)
        return m_qListHemispheres[1];
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return m_qListHemispheres[0];
    }
}

//=============================================================================================================

const MNEHemisphere& MNESourceSpace::operator[] (QString idt) const
{
    if(idt.compare("lh") == 0)
        return m_qListHemispheres[0];
    else if(idt.compare("rh") == 0)
        return m_qListHemispheres[1];
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return m_qListHemispheres[0];
    }
}
