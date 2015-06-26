//=============================================================================================================
/**
* @file     mne_bem.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_bem.h"

#include <utils/mnemath.h>
#include <fs/label.h>
#include <fstream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FSLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEBem::MNEBem()
{
}

//*************************************************************************************************************

MNEBem::MNEBem(const MNEBem &p_MNEBem)
: m_qListBemSurface(p_MNEBem.m_qListBemSurface)
{

}


//*************************************************************************************************************

MNEBem::MNEBem(QIODevice &p_IODevice)   //const MNESourceSpace &p_MNESourceSpace
//: m_qListBemSurface()
{
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    FiffDirTree t_Tree;
//    QList<FiffDirEntry>fiffDirEntries;

//    if(!t_pStream->open(t_Tree, fiffDirEntries))
//    {
//        qCritical() << "Could not open FIFF stream!";
////        return false;
//    }


    if(!MNEBem::readFromStream(t_pStream, true, t_Tree, *this))
    {
        t_pStream->device()->close();
        std::cout << "Could not read the source spaces\n"; // ToDo throw error
        //ToDo error(me,'Could not read the source spaces (%s)',mne_omit_first_line(lasterr));
//        return false;
    }

}


//*************************************************************************************************************

MNEBem::~MNEBem()
{

}

//*************************************************************************************************************

bool MNEBem::readFromStream(FiffStream::SPtr& p_pStream, bool add_geom, FiffDirTree& p_Tree, MNEBem& p_Bem)
{

    //
    //   Open the file, create directory
    //
    bool open_here = false;
    if (!p_pStream->device()->isOpen())
    {
        QList<FiffDirEntry> t_Dir;
        QString t_sFileName = p_pStream->streamName();

        QFile t_file(t_sFileName);//ToDo TCPSocket;
        p_pStream = FiffStream::SPtr(new FiffStream(&t_file));
        if(!p_pStream->open(p_Tree, t_Dir))
            return false;
        open_here = true;
//        if(t_pDir)
//            delete t_pDir;


        //
        //   Find all BEM surfaces
        //

        QList<FiffDirTree>bem = p_Tree.dir_tree_find(FIFFB_BEM);
        if(bem.isEmpty())
        {
            qCritical() << "No BEM block found!";
            return false;
        }

        QList<FiffDirTree>bemsurf = p_Tree.dir_tree_find(FIFFB_BEM_SURF);
        if(bemsurf.isEmpty())
        {
            qCritical() << "No BEM surfaces found!";
            return false;
        }

        for(int k = 0; k < bemsurf.size(); ++k)
        {
            MNEBemSurface  p_BemSurface;
            printf("\tReading a BEM surface...");
            MNEBem::read_bem_surface(p_pStream.data(), bemsurf[k], p_BemSurface);
             if (add_geom)
                complete_surface_info(p_BemSurface);
            printf("\t[done]\n" );

            p_Bem.m_qListBemSurface.append(p_BemSurface);

    //           src(k) = this;
        }
    }
return true;
}

bool MNEBem::read_bem_surface(FiffStream *p_pStream, const FiffDirTree &p_Tree, MNEBemSurface &p_BemSurface)
{
    p_BemSurface.clear();

    FiffTag::SPtr t_pTag;

    //=====================================================================
    if(!p_Tree.find_tag(p_pStream, FIFF_BEM_SURF_ID, t_pTag))
         p_BemSurface.id = FIFFV_BEM_SURF_ID_UNKNOWN;
    else
         p_BemSurface.id = *t_pTag->toInt();

        qDebug() << "Read SourceSpace ID; type:" << t_pTag->getType() << "value:" << *t_pTag->toInt();

    //=====================================================================
    if(!p_Tree.find_tag(p_pStream, FIFF_BEM_SIGMA, t_pTag))
         p_BemSurface.sigma = 1.0;
    else
         p_BemSurface.sigma = *t_pTag->toFloat();

//        qDebug() <<

    //=====================================================================
    if(!p_Tree.find_tag(p_pStream, FIFF_BEM_SURF_NNODE, t_pTag))
    {
        qCritical() << "np not found!";
        return false;
    }
    else
         p_BemSurface.np = *t_pTag->toInt();

//        qDebug() <<

    //=====================================================================
    if(!p_Tree.find_tag(p_pStream, FIFF_BEM_SURF_NTRI, t_pTag))
    {
        qCritical() << "ntri not found!";
        return false;
    }
    else
         p_BemSurface.ntri = *t_pTag->toInt();

//        qDebug() <<

    //=====================================================================
    if(!p_Tree.find_tag(p_pStream, FIFF_MNE_COORD_FRAME, t_pTag))
    {
        qWarning()
                        << "FIFF_MNE_COORD_FRAME not found, trying FIFF_BEM_COORD_FRAME.";
        if(!p_Tree.find_tag(p_pStream, FIFF_BEM_COORD_FRAME, t_pTag))
        {
            p_pStream->device()->close();
            std::cout << "Coordinate frame information not found."; //ToDo: throw error.
            return false;
        }
        else
            p_BemSurface.coord_frame = *t_pTag->toInt();
    }
    else
         p_BemSurface.coord_frame = *t_pTag->toInt();

//        qDebug() <<

    //=====================================================================
    //
    //   Vertices, normals, and triangles
    //
    //=====================================================================
    if(!p_Tree.find_tag(p_pStream, FIFF_BEM_SURF_NODES, t_pTag))
    {
        p_pStream->device()->close();
        std::cout << "Vertex data not found."; //ToDo: throw error.
        return false;
    }

    p_BemSurface.rr = t_pTag->toFloatMatrix().transpose();
    qint32 rows_rr = p_BemSurface.rr.rows();
        qDebug() << "last element rr: " << p_BemSurface.rr(rows_rr-1, 0) << p_BemSurface.rr(rows_rr-1, 1) << p_BemSurface.rr(rows_rr-1, 2);

    if (rows_rr != p_BemSurface.np)
    {
        p_pStream->device()->close();
        std::cout << "Vertex information is incorrect."; //ToDo: throw error.
        return false;
    }
        qDebug() << "Surf Nodes; type:" << t_pTag->getType();


    //=====================================================================
    if(!p_Tree.find_tag(p_pStream, FIFF_BEM_SURF_NORMALS, t_pTag))
    {
        if(!p_Tree.find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NORMALS, t_pTag))
        {
            p_pStream->device()->close();
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
        p_pStream->device()->close();
        std::cout << "Vertex normal information is incorrect."; //ToDo: throw error.
        return false;
    }


        qDebug() << "Source Space Normals; type:" << t_pTag->getType();


    //=====================================================================
    if (p_BemSurface.ntri > 0)
    {
        if(!p_Tree.find_tag(p_pStream, FIFF_BEM_SURF_TRIANGLES, t_pTag))
        {
            if(!p_Tree.find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_TRIANGLES, t_pTag))
            {
                p_pStream->device()->close();
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
            p_pStream->device()->close();
            std::cout << "Triangulation information is incorrect."; //ToDo: throw error.
            return false;
        }
    }
    else
    {
        MatrixXi p_defaultMatrix(0, 0);
        p_BemSurface.tris = p_defaultMatrix;
    }
        qDebug() << "Triangles; type:" << t_pTag->getType() << "rows:" << p_BemSurface.tris.rows() << "cols:" << p_BemSurface.tris.cols();
        qDebug() << "First Triangle: " << p_BemSurface.tris(0, 0) << p_BemSurface.tris(0, 1) << p_BemSurface.tris(0, 2);
        qDebug() << "Last Triangle: " << p_BemSurface.tris(p_BemSurface.tris.rows()-1, 0) << p_BemSurface.tris(p_BemSurface.tris.rows()-1, 1) << p_BemSurface.tris(p_BemSurface.tris.rows()-1, 2);


    return true;
}


bool MNEBem::complete_surface_info(MNEBemSurface& p_BemSurf)
{
    //
    //   Main triangulation
    //
    printf("\tCompleting triangulation info...");
    p_BemSurf.tri_cent = MatrixX3d::Zero(p_BemSurf.ntri,3);
    p_BemSurf.tri_nn = MatrixX3d::Zero(p_BemSurf.ntri,3);
    p_BemSurf.tri_area = VectorXd::Zero(p_BemSurf.ntri);

    Matrix3d r;
    Vector3d a, b;
    int k = 0;
    float size = 0;
    for (qint32 i = 0; i < p_BemSurf.ntri; ++i)
    {
        for ( qint32 j = 0; j < 3; ++j)
        {
            k = p_BemSurf.tris(i, j);

            r(j,0) = p_BemSurf.rr(k, 0);
            r(j,1) = p_BemSurf.rr(k, 1);
            r(j,2) = p_BemSurf.rr(k, 2);

            p_BemSurf.tri_cent(i, 0) += p_BemSurf.rr(k, 0);
            p_BemSurf.tri_cent(i, 1) += p_BemSurf.rr(k, 1);
            p_BemSurf.tri_cent(i, 2) += p_BemSurf.rr(k, 2);
        }
        p_BemSurf.tri_cent.row(i) /= 3.0f;

        //cross product {cross((r2-r1),(r3-r1))}
        a = r.row(1) - r.row(0 );
        b = r.row(2) - r.row(0);
        p_BemSurf.tri_nn(i,0) = a(1)*b(2)-a(2)*b(1);
        p_BemSurf.tri_nn(i,1) = a(2)*b(0)-a(0)*b(2);
        p_BemSurf.tri_nn(i,2) = a(0)*b(1)-a(1)*b(0);

        //area
        size = p_BemSurf.tri_nn.row(i)*p_BemSurf.tri_nn.row(i).transpose();
        size = std::pow(size, 0.5f );

        p_BemSurf.tri_area(i) = size/2.0f;
        p_BemSurf.tri_nn.row(i) /= size;


    }
    printf("[done]\n");

//        qDebug() << "p_BemSurf.tri_cent:" << p_BemSurf.tri_cent(0,0) << p_BemSurf.tri_cent(0,1) << p_BemSurf.tri_cent(0,2);
//        qDebug() << "p_BemSurf.tri_cent:" << p_BemSurf.tri_cent(2,0) << p_BemSurf.tri_cent(2,1) << p_BemSurf.tri_cent(2,2);

        qDebug() << "p_BemSurf.tri_nn:" << p_BemSurf.tri_nn(0,0) << p_BemSurf.tri_nn(0,1) << p_BemSurf.tri_nn(0,2);
        qDebug() << "p_BemSurf.tri_nn:" << p_BemSurf.tri_nn(2,0) << p_BemSurf.tri_nn(2,1) << p_BemSurf.tri_nn(2,2);

//        //
//        //   Accumulate the vertex normals
//        //

        MatrixX3f nnTest;
        nnTest = MatrixX3f::Zero(p_BemSurf.np,3);

        for (qint32 p = 0; p < p_BemSurf.ntri; p++)
        {
            for (qint32 j=0; j<3 ; j++)
            {
                int nodenr;
                nodenr = p_BemSurf.tris(p,j);
                nnTest(nodenr,0) += p_BemSurf.tri_nn(p,0);
                nnTest(nodenr,1) += p_BemSurf.tri_nn(p,1);
                nnTest(nodenr,2) += p_BemSurf.tri_nn(p,2);
            }
        }

            // normalize
        for (qint32 p = 0; p < p_BemSurf.np; p++)
        {
            size = nnTest.row(p)*nnTest.row(p).transpose();
            size = std::pow(size, 0.5f );
            nnTest.row(p) /= size;
        }

 qDebug() << "nnTest first Row:" << nnTest(0,0)<<nnTest(0,1)<<nnTest(0,2);

 get_EigentoData(nnTest, "nntest.txt");

    return true;
}

//void MNEBem::matrixtofile()
//{
//  std::ofstream file("test.txt");
//  if (file.is_open())
//  {
//    MatrixXf m = MatrixXf::Random(30,3);
//    file << "Here is the matrix m:\n" << m << '\n';
//    file << "m" << '\n' <<  colm(m) << '\n';
//  }
//}

void MNEBem::get_EigentoData(MatrixX3f& src, char* pathAndName)
{
      std::fstream doc(pathAndName, std::ofstream::out | std::ofstream::trunc);
      if(doc)  // if succesfully opened
      {
        // instructions
        doc << "Here is the matrix src:\n" << src << "\n";
        doc.close();
      }
      else
      {
        std::cerr << "Error when opening" << std::endl;
      }
 }

