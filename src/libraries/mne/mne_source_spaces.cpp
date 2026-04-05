//=============================================================================================================
/**
 * @file     mne_source_spaces.cpp
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
 * @brief    MNESourceSpaces class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_source_spaces.h"
#include "mne_nearest.h"

#include <math/linalg.h>
#include <fs/fs_label.h>

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

MNESourceSpaces::MNESourceSpaces()
{
}

//=============================================================================================================

MNESourceSpaces::MNESourceSpaces(const MNESourceSpaces &p_MNESourceSpaces)
{
    m_sourceSpaces.reserve(p_MNESourceSpaces.m_sourceSpaces.size());
    for (const auto& sp : p_MNESourceSpaces.m_sourceSpaces)
        m_sourceSpaces.push_back(sp->clone());
}

//=============================================================================================================

MNESourceSpaces::~MNESourceSpaces()
{
}

//=============================================================================================================

void MNESourceSpaces::clear()
{
    m_sourceSpaces.clear();
}

//=============================================================================================================

void MNESourceSpaces::append(const MNESourceSpace& space)
{
    m_sourceSpaces.push_back(space.clone());
}

//=============================================================================================================

QList<VectorXi> MNESourceSpaces::get_vertno() const
{
    QList<VectorXi> p_vertices;
    for(qint32 i = 0; i < static_cast<qint32>(m_sourceSpaces.size()); ++i)
        p_vertices.push_back(m_sourceSpaces[i]->vertno);
    return p_vertices;
}

//=============================================================================================================

QList<VectorXi> MNESourceSpaces::label_src_vertno_sel(const FsLabel &p_label, VectorXi &src_sel) const
{
//    if(src[0].['type'] != 'surf')
//        return Exception('FsLabel are only supported with surface source spaces')

    QList<VectorXi> vertno;
    vertno << this->m_sourceSpaces[0]->vertno << this->m_sourceSpaces[1]->vertno;

    if (p_label.hemi == 0) //lh
    {
        VectorXi vertno_sel = Linalg::intersect(vertno[0], p_label.vertices, src_sel);
        vertno[0] = vertno_sel;
        vertno[1] = VectorXi();
    }
    else if (p_label.hemi == 1) //rh
    {
        VectorXi vertno_sel = Linalg::intersect(vertno[1], p_label.vertices, src_sel);
        src_sel.array() += p_label.vertices.size();
        vertno[0] = VectorXi();
        vertno[1] = vertno_sel;
    }

//    if (p_label.hemi == 0) //lh
//    {
//        VectorXi vertno_sel = Linalg::intersect(vertno[0], p_label.vertices[0], src_sel);
//        vertno[0] = vertno_sel;
//        vertno[1] = VectorXi();
//    }
//    else if (p_label.hemi == 1) //rh
//    {
//        VectorXi vertno_sel = Linalg::intersect(vertno[1], p_label.vertices[1], src_sel);
//        src_sel.array() += p_label.vertices[0].size();
//        vertno[0] = VectorXi();
//        vertno[1] = vertno_sel;
//    }
//    else if (p_label.hemi == 2) //both
//    {
//        VectorXi src_sel_lh, src_sel_rh;
//        VectorXi vertno_sel_lh = Linalg::intersect(vertno[0], p_label.vertices[0], src_sel_lh);
//        VectorXi vertno_sel_rh = Linalg::intersect(vertno[1], p_label.vertices[1], src_sel_rh);
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

MNESourceSpaces MNESourceSpaces::pick_regions(const QList<FsLabel> &p_qListLabels) const
{
    Q_UNUSED(p_qListLabels);

    MNESourceSpaces selectedSrc(*this);

    for(qint32 h = 0; h < 2; ++h)
    {
        auto& srcSpace = *selectedSrc.m_sourceSpaces[h];
        const auto& origSpace = *this->m_sourceSpaces[h];
        auto* selHemi = dynamic_cast<MNEHemisphere*>(&srcSpace);
        auto* origHemi = dynamic_cast<const MNEHemisphere*>(&origSpace);

        VectorXi selVertices;

        //get vertices indeces for new selection
        qint32 iSize = 0;
        for(qint32 i = 0; i < p_qListLabels.size(); ++i)
        {
            if(p_qListLabels[i].hemi == h)
            {
                VectorXi currentSelection;

                Linalg::intersect(origSpace.vertno, p_qListLabels[i].vertices, currentSelection);

                selVertices.conservativeResize(iSize+currentSelection.size());
                selVertices.block(iSize,0,currentSelection.size(),1) = currentSelection;
                iSize = selVertices.size();
            }
        }

        Linalg::sort(selVertices, false);

        VectorXi newVertno(selVertices.size());

        srcSpace.inuse = VectorXi::Zero(srcSpace.np);

        for(qint32 i = 0; i < selVertices.size(); ++i)
        {
            srcSpace.inuse[selVertices[i]] = 1;
            newVertno[i] = origSpace.vertno[selVertices[i]];
        }

        srcSpace.nuse = selVertices.size();
        srcSpace.vertno = newVertno;

        //
        // Tris
        //
        VectorXi idx_select = VectorXi::Zero(origSpace.use_itris.rows());
        for(qint32 i = 0; i < 3; ++i)
        {
            VectorXi tri_dim = origSpace.use_itris.col(i);
            VectorXi idx_dim;
            Linalg::intersect(tri_dim, newVertno, idx_dim);

            for(qint32 j = 0; j < idx_dim.size(); ++j)
                idx_select[idx_dim[j]] = 1;
        }

        qint32 countSel = 0;
        for(qint32 i = 0; i < idx_select.size(); ++i)
            if(idx_select[i] == 1)
                ++countSel;

        srcSpace.nuse_tri = countSel;

        MatrixX3i use_tris_new(countSel,3);
        MatrixX3d use_tri_cent_new(countSel,3);
        MatrixX3d use_tri_nn_new(countSel,3);
        VectorXd use_tri_area_new(countSel);

        countSel = 0;
        for(qint32 i = 0; i < idx_select.size(); ++i)
        {
            if(idx_select[i] == 1)
            {
                use_tris_new.row(countSel) = origSpace.use_itris.row(i);
                if (origHemi && selHemi) {
                    use_tri_cent_new.row(countSel) = origHemi->use_tri_cent.row(i);
                    use_tri_nn_new.row(countSel) = origHemi->use_tri_nn.row(i);
                    use_tri_area_new[countSel] = origHemi->use_tri_area[i];
                }
                ++countSel;
            }
        }

        srcSpace.use_itris = use_tris_new;
        if (selHemi) {
            selHemi->use_tri_cent = use_tri_cent_new;
            selHemi->use_tri_nn = use_tri_nn_new;
            selHemi->use_tri_area = use_tri_area_new;
        }
    }

    return selectedSrc;
}

//=============================================================================================================

bool MNESourceSpaces::readFromStream(FiffStream::SPtr& p_pStream,
                                    bool add_geom,
                                    MNESourceSpaces& p_SourceSpace)
{
//    if (p_pSourceSpace != NULL)
//        delete p_pSourceSpace;
    p_SourceSpace = MNESourceSpaces();

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
        auto p_Hemisphere = std::make_shared<MNEHemisphere>();
        qInfo("\tReading a source space...");
        MNESourceSpaces::read_source_space(p_pStream, spaces[k], *p_Hemisphere);
        qInfo("\t[done]\n" );
        if (add_geom)
            p_Hemisphere->complete_source_space_info();

        p_SourceSpace.m_sourceSpaces.push_back(p_Hemisphere);

//           src(k) = this;
    }

    qInfo("\t%lld source spaces read\n", spaces.size());

    if(open_here)
        p_pStream->close();

    return true;
}

//=============================================================================================================

qint32 MNESourceSpaces::find_source_space_hemi(MNESourceSpace& p_SourceSpace)
{
    return p_SourceSpace.find_source_space_hemi();
}

//=============================================================================================================

bool MNESourceSpaces::transform_source_space_to(fiff_int_t dest, FiffCoordTrans& trans)
{
    for(size_t k = 0; k < this->m_sourceSpaces.size(); ++k)
    {
        auto* hemi = dynamic_cast<MNEHemisphere*>(m_sourceSpaces[k].get());
        if(hemi) {
            if(!hemi->transform_hemisphere_to(dest,trans))
            {
                qWarning("Could not transform source space.");
                return false;
            }
        }
    }
    return true;
}

//=============================================================================================================

bool MNESourceSpaces::read_source_space(FiffStream::SPtr& p_pStream, const FiffDirNode::SPtr& p_Tree, MNEHemisphere& p_Hemisphere)
{
    p_Hemisphere.clear();

    FiffTag::UPtr t_pTag;

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
                p_Hemisphere.itris = t_pTag->toIntMatrix().transpose();
                p_Hemisphere.itris.array() -= 1;//0 based indizes
            }
        }
        else
        {
            p_Hemisphere.itris = t_pTag->toIntMatrix().transpose();
            p_Hemisphere.itris.array() -= 1;//0 based indizes
        }
        if (p_Hemisphere.itris.rows() != p_Hemisphere.ntri)
        {
            p_pStream->close();
            std::cout << "Triangulation information is incorrect."; //ToDo: throw error.
            return false;
        }
    }
    else
    {
        p_Hemisphere.itris.resize(0, 3);
    }
//        qDebug() << "Triangles; type:" << t_pTag->getType() << "rows:" << p_Hemisphere.itris.rows() << "cols:" << p_Hemisphere.itris.cols();

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
    FiffTag::UPtr t_pTag1;
    FiffTag::UPtr t_pTag2;
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NUSE_TRI, t_pTag1) || !p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, t_pTag2))
    {
        MatrixX3i p_defaultMatrix;
        p_Hemisphere.nuse_tri = 0;
        p_Hemisphere.use_itris = p_defaultMatrix;
    }
    else
    {
        p_Hemisphere.nuse_tri = *t_pTag1->toInt();
        p_Hemisphere.use_itris = t_pTag2->toIntMatrix().transpose();
        p_Hemisphere.use_itris.array() -= 1; //0 based indizes
    }
//        qDebug() << "triangulation; type:" << t_pTag2->getType() << "use_itris:" << p_Hemisphere.use_itris.rows()<< "x" << p_Hemisphere.use_itris.cols();

    //
    //   Patch-related information
    //
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NEAREST, t_pTag1) || !p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, t_pTag2))
    {
        p_Hemisphere.nearest.clear();
    }
    else
    {
       //res.nearest = tag1.data + 1;
       VectorXi nearestIdx = VectorXi(Map<VectorXi>(t_pTag1->toInt(), t_pTag1->size()/4, 1));
       VectorXd nearestDist = VectorXd((Map<VectorXf>(t_pTag2->toFloat(), t_pTag2->size()/4, 1)).cast<double>());
       p_Hemisphere.setNearestData(nearestIdx, nearestDist);
    }

//    patch_info(p_Hemisphere.nearest, p_Hemisphere.pinfo);
    if (p_Hemisphere.compute_patch_info())
       qInfo("\tPatch information added...");
    //
    // Distances
    //
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_DIST, t_pTag1) || !p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, t_pTag2))
    {
       p_Hemisphere.dist = FiffSparseMatrix();
       p_Hemisphere.dist_limit = 0;
    }
    else
    {
        auto dist_lower = FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag1);
        if (dist_lower) {
            auto dist_full = dist_lower->mne_add_upper_triangle_rcs();
            if (dist_full) {
                p_Hemisphere.dist = std::move(*dist_full);
            }
        }
        p_Hemisphere.dist_limit = *t_pTag2->toFloat(); //ToDo Check if this is realy always a float and not a matrix
    }

    return true;
}

//=============================================================================================================

bool MNESourceSpaces::patch_info(MNEHemisphere &p_Hemisphere)
{
    return p_Hemisphere.compute_patch_info();
}

//=============================================================================================================

bool MNESourceSpaces::complete_source_space_info(MNEHemisphere& p_Hemisphere)
{
    return p_Hemisphere.complete_source_space_info();
}

//=============================================================================================================

void MNESourceSpaces::writeToStream(FiffStream* p_pStream)
{
    for(size_t h = 0; h < m_sourceSpaces.size(); ++h)
    {
        qInfo("\tWrite a source space... ");
        p_pStream->start_block(FIFFB_MNE_SOURCE_SPACE);
        auto* hemi = dynamic_cast<MNEHemisphere*>(m_sourceSpaces[h].get());
        if (hemi)
            hemi->writeToStream(p_pStream);
        p_pStream->end_block(FIFFB_MNE_SOURCE_SPACE);
        qInfo("[done]\n");
    }
    qInfo("\t%zu source spaces written\n", m_sourceSpaces.size());
}

//=============================================================================================================

MNESourceSpace& MNESourceSpaces::operator[] (qint32 idx)
{
    if(static_cast<qint32>(m_sourceSpaces.size()) > idx)
        return *m_sourceSpaces[idx];
    else
    {
        qWarning("Warning: Index out of bound! Returning last element.");
        return *m_sourceSpaces.back();
    }
}

//=============================================================================================================

const MNESourceSpace& MNESourceSpaces::operator[] (qint32 idx) const
{
    if(static_cast<qint32>(m_sourceSpaces.size()) > idx)
        return *m_sourceSpaces[idx];
    else
    {
        qWarning("Warning: Index out of bound! Returning last element.");
        return *m_sourceSpaces.back();
    }
}

//=============================================================================================================

MNESourceSpace& MNESourceSpaces::operator[] (QString idt)
{
    if(idt.compare("lh") == 0)
        return *m_sourceSpaces[0];
    else if(idt.compare("rh") == 0)
        return *m_sourceSpaces[1];
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return *m_sourceSpaces[0];
    }
}

//=============================================================================================================

const MNESourceSpace& MNESourceSpaces::operator[] (QString idt) const
{
    if(idt.compare("lh") == 0)
        return *m_sourceSpaces[0];
    else if(idt.compare("rh") == 0)
        return *m_sourceSpaces[1];
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return *m_sourceSpaces[0];
    }
}

//=============================================================================================================

MNEHemisphere* MNESourceSpaces::hemisphereAt(qint32 idx)
{
    if(idx >= 0 && idx < static_cast<qint32>(m_sourceSpaces.size()))
        return dynamic_cast<MNEHemisphere*>(m_sourceSpaces[idx].get());
    return nullptr;
}

//=============================================================================================================

const MNEHemisphere* MNESourceSpaces::hemisphereAt(qint32 idx) const
{
    if(idx >= 0 && idx < static_cast<qint32>(m_sourceSpaces.size()))
        return dynamic_cast<const MNEHemisphere*>(m_sourceSpaces[idx].get());
    return nullptr;
}

//=============================================================================================================

std::shared_ptr<MNESourceSpace>& MNESourceSpaces::at(qint32 idx)
{
    return m_sourceSpaces.at(idx);
}

//=============================================================================================================

const std::shared_ptr<MNESourceSpace>& MNESourceSpaces::at(qint32 idx) const
{
    return m_sourceSpaces.at(idx);
}
