//=============================================================================================================
/**
 * @file     mne_bem.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNEBem class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_bem.h"

#include <utils/mnemath.h>
#include <utils/warp.h>
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
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEBem::MNEBem()
{
}

//=============================================================================================================

MNEBem::MNEBem(const MNEBem &p_MNEBem)
: m_qListBemSurface(p_MNEBem.m_qListBemSurface)
{
}

//=============================================================================================================

MNEBem::MNEBem(QIODevice &p_IODevice)   //const MNEBem &p_MNEBem
//: m_qListBemSurface()
{
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));

    if(!MNEBem::readFromStream(t_pStream, true, *this))
    {
        t_pStream->close();
        std::cout << "Could not read the bem surfaces\n"; // ToDo throw error
        //ToDo error(me,'Could not read the bem surfaces (%s)',mne_omit_first_line(lasterr));
//        return false;
    }

//    bool testStream =t_pStream->device()->isOpen();
}

//=============================================================================================================

MNEBem::~MNEBem()
{
}

//=============================================================================================================

void MNEBem::clear()
{
    m_qListBemSurface.clear();
}

//=============================================================================================================

bool MNEBem::readFromStream(FiffStream::SPtr& p_pStream, bool add_geom, MNEBem& p_Bem)
{
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
        {
            return false;
        }
        open_here = true;
//        if(t_pDir)
//            delete t_pDir;
    }

    //
    //   Find all BEM surfaces
    //

    QList<FiffDirNode::SPtr> bem = p_pStream->dirtree()->dir_tree_find(FIFFB_BEM);
    if(bem.isEmpty())
    {
        qCritical() << "No BEM block found!";
        if(open_here)
        {
            p_pStream->close();
        }
        return false;
    }

    QList<FiffDirNode::SPtr> bemsurf = p_pStream->dirtree()->dir_tree_find(FIFFB_BEM_SURF);
    if(bemsurf.isEmpty())
    {
        qCritical() << "No BEM surfaces found!";
        if(open_here)
        {
            p_pStream->close();
        }
        return false;
    }

    for(int k = 0; k < bemsurf.size(); ++k)
    {
        MNEBemSurface  p_BemSurface;
        printf("\tReading a BEM surface...");
        MNEBem::readBemSurface(p_pStream, bemsurf[k], p_BemSurface);
        p_BemSurface.addTriangleData();
        if (add_geom)
        {
           p_BemSurface.addVertexNormals();
        }
        printf("\t[done]\n" );

        p_Bem.m_qListBemSurface.append(p_BemSurface);
//           src(k) = this;
    }

    printf("\t%d bem surfaces read\n", bemsurf.size());

    if(open_here)
    {
        p_pStream->close();
    }
    return true;
}

//=============================================================================================================

bool MNEBem::readBemSurface(FiffStream::SPtr& p_pStream, const FiffDirNode::SPtr &p_Tree, MNEBemSurface &p_BemSurface)
{
    p_BemSurface.clear();

    FiffTag::SPtr t_pTag;

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_BEM_SURF_ID, t_pTag))
    {
         p_BemSurface.id = FIFFV_BEM_SURF_ID_UNKNOWN;
    }
    else
    {
         p_BemSurface.id = *t_pTag->toInt();
    }

//    qDebug() << "Read BemSurface ID; type:" << t_pTag->getType() << "value:" << *t_pTag->toInt();

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_BEM_SIGMA, t_pTag))
    {
         p_BemSurface.sigma = 1.0;
    }
    else
    {
         p_BemSurface.sigma = *t_pTag->toFloat();
    }

//    qDebug() <<

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_BEM_SURF_NNODE, t_pTag))
    {
        p_pStream->close();
        std::cout << "np not found!";
        return false;
    }
    else
    {
         p_BemSurface.np = *t_pTag->toInt();
    }

//    qDebug() <<

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_BEM_SURF_NTRI, t_pTag))
    {
        p_pStream->close();
        std::cout << "ntri not found!";
        return false;
    }
    else
    {
         p_BemSurface.ntri = *t_pTag->toInt();
    }

//    qDebug() <<

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_MNE_COORD_FRAME, t_pTag))
    {
        qWarning() << "FIFF_MNE_COORD_FRAME not found, trying FIFF_BEM_COORD_FRAME.";
        if(!p_Tree->find_tag(p_pStream, FIFF_BEM_COORD_FRAME, t_pTag))
        {
            p_pStream->close();
            std::cout << "Coordinate frame information not found."; //ToDo: throw error.
            return false;
        }
        else
        {
            p_BemSurface.coord_frame = *t_pTag->toInt();
        }
    }
    else
    {
         p_BemSurface.coord_frame = *t_pTag->toInt();
    }

//    qDebug() <<

    //=====================================================================
    //
    //   Vertices, normals, and triangles
    //
    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_BEM_SURF_NODES, t_pTag))
    {
        p_pStream->close();
        std::cout << "Vertex data not found."; //ToDo: throw error.
        return false;
    }

    p_BemSurface.rr = t_pTag->toFloatMatrix().transpose();
    qint32 rows_rr = p_BemSurface.rr.rows();

    if (rows_rr != p_BemSurface.np)
    {
        p_pStream->close();
        std::cout << "Vertex information is incorrect."; //ToDo: throw error.
        return false;
    }

//    qDebug() << "Surf Nodes; type:" << t_pTag->getType();

    //=====================================================================
    if(!p_Tree->find_tag(p_pStream, FIFF_BEM_SURF_NORMALS, t_pTag))
    {
        if(!p_Tree->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NORMALS, t_pTag))
        {
            p_pStream->close();
            std::cout << "Vertex normals not found."; //ToDo: throw error.
            return false;
        }

        p_BemSurface.nn = t_pTag->toFloatMatrix().transpose();
    }
    else
    {
        p_BemSurface.nn = t_pTag->toFloatMatrix().transpose();
    }

    if (p_BemSurface.nn.rows() != p_BemSurface.np)
    {
        p_pStream->close();
        std::cout << "Vertex normal information is incorrect."; //ToDo: throw error.
        return false;
    }

//    qDebug() << "Bem Vertex Normals; type:" << t_pTag->getType();

    //=====================================================================
    if (p_BemSurface.ntri > 0)
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
                p_BemSurface.tris = t_pTag->toIntMatrix().transpose();
                p_BemSurface.tris -= MatrixXi::Constant(p_BemSurface.tris.rows(),3,1);//0 based indizes
            }
        }
        else
        {
            p_BemSurface.tris = t_pTag->toIntMatrix().transpose();
            p_BemSurface.tris -= MatrixXi::Constant(p_BemSurface.tris.rows(),3,1);//0 based indizes
        }

        if (p_BemSurface.tris.rows() != p_BemSurface.ntri)
        {
            p_pStream->close();
            std::cout << "Triangulation information is incorrect."; //ToDo: throw error.
            return false;
        }
    }
    else
    {
        MatrixXi p_defaultMatrix(0, 0);
        p_BemSurface.tris = p_defaultMatrix;
    }

    return true;
}

//=============================================================================================================

void MNEBem::write(QIODevice &p_IODevice)
{
    //
    //   Open the file, create directory
    //

    // Create the file and save the essentials
    FiffStream::SPtr t_pStream = FiffStream::start_file(p_IODevice);
    printf("Write BEM surface in %s...\n", t_pStream->streamName().toUtf8().constData());
    this->writeToStream(t_pStream.data());
    t_pStream->end_file();
}

//=============================================================================================================

void MNEBem::writeToStream(FiffStream* p_pStream)
{
    p_pStream->start_block(FIFFB_BEM);
    for(qint32 h = 0; h < m_qListBemSurface.size(); ++h)
    {
        printf("\tWrite a bem surface... ");
        p_pStream->start_block(FIFFB_BEM_SURF);
        m_qListBemSurface[h].writeToStream(p_pStream);
        p_pStream->end_block(FIFFB_BEM_SURF);
        printf("[done]\n");
    }
    printf("\t%d bem surfaces written\n", m_qListBemSurface.size());
    p_pStream->end_block(FIFFB_BEM);
}

//=============================================================================================================

const MNEBemSurface& MNEBem::operator[] (qint32 idx) const
{
    if (idx>=m_qListBemSurface.length())
    {
        qWarning("Warning: Required surface doesn't exist! Returning surface '0'.");
        idx=0;
    }
    return m_qListBemSurface[idx];
}

//=============================================================================================================

MNEBemSurface& MNEBem::operator[] (qint32 idx)
{
    if (idx >= m_qListBemSurface.length())
    {
        qWarning("Warning: Required surface doesn't exist! Returning surface '0'.");
        idx = 0;
    }
    return m_qListBemSurface[idx];
}

//=============================================================================================================

MNEBem &MNEBem::operator<<(const MNEBemSurface &surf)
{
    this->m_qListBemSurface.append(surf);
    return *this;
}

//=============================================================================================================

MNEBem &MNEBem::operator<<(const MNEBemSurface *surf)
{
    this->m_qListBemSurface.append(*surf);
    return *this;
}

//=============================================================================================================

void MNEBem::warp(const MatrixXf & sLm, const MatrixXf &dLm)
{
    Warp help;
    QList<MatrixXf> vertList;
    for (int i=0; i<this->m_qListBemSurface.size(); i++)
    {
        vertList.append(this->m_qListBemSurface[i].rr);
    }

    help.calculate(sLm, dLm, vertList);

    for (int i=0; i<this->m_qListBemSurface.size(); i++)
    {
        this->m_qListBemSurface[i].rr = vertList.at(i);
    }
    return;
}

//=============================================================================================================

void MNEBem::transform(const FiffCoordTrans& trans)
{
    MatrixX3f vert;
    for (int i=0; i<this->m_qListBemSurface.size(); i++)
    {
        vert = this->m_qListBemSurface[i].rr;
        vert = trans.apply_trans(vert);
        this->m_qListBemSurface[i].rr = vert;
    }
    return;
}

//=============================================================================================================

void MNEBem::invtransform(const FiffCoordTrans& trans)
{
    MatrixX3f vert;
    for (int i=0; i<this->m_qListBemSurface.size(); i++)
    {
        vert = this->m_qListBemSurface[i].rr;
        vert = trans.apply_inverse_trans(vert);
        this->m_qListBemSurface[i].rr = vert;
    }
    return;
}
