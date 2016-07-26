//=============================================================================================================
/**
* @file     fiff_dig_point.cpp
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
* @brief    Implementation of the FiffDigPoint Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_point.h"
#include "fiff_coord_trans.h"
#include "fiff_stream.h"
#include "fiff_dir_tree.h"
#include "fiff_tag.h"



//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDigPoint::FiffDigPoint()
: kind(-1)
, ident(-1)
, coord_frame(-1)
{
    for(qint32 i = 0; i < 3; ++i)
        r[i] = -1;
}


//*************************************************************************************************************

FiffDigPoint::FiffDigPoint(const FiffDigPoint& p_FiffDigPoint)
: kind(p_FiffDigPoint.kind)
, ident(p_FiffDigPoint.ident)
, coord_frame(p_FiffDigPoint.coord_frame)
{
    for(qint32 i = 0; i < 3; ++i)
        r[i] = p_FiffDigPoint.r[i];
}


//*************************************************************************************************************

FiffDigPoint::~FiffDigPoint()
{

}


//*************************************************************************************************************

QList<FiffDigPoint> FiffDigPoint::read(QIODevice &p_fileDig)
{
    QList<FiffDigPoint> dig;
    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_fileDig));
    QString t_sFileName = t_pStream->streamName();

    printf("Opening header data %s...\n",t_sFileName.toUtf8().constData());

    FiffDirTree t_Tree;
    QList<FiffDirEntry> t_Dir;
    if(!t_pStream->open(t_Tree, t_Dir))
    {
        qDebug()<<"Can not open the Electrode File";
        return dig;
    }
    //
    //   Read the measurement info
    //
    //read_hpi_info(t_pStream,t_Tree, info);
    fiff_int_t kind = -1;
    fiff_int_t pos = -1;
    FiffTag::SPtr t_pTag;

    //
    //   Locate the Electrodes
    //
    QList<FiffDirTree> isotrak = t_Tree.dir_tree_find(FIFFB_ISOTRAK);

    fiff_int_t coord_frame = FIFFV_COORD_HEAD;
    FiffCoordTrans dig_trans;
    qint32 k = 0;

    if (isotrak.size() == 1)
    {
        for (k = 0; k < isotrak[0].nent; ++k)
        {
            kind = isotrak[0].dir[k].kind;
            pos  = isotrak[0].dir[k].pos;
            if (kind == FIFF_DIG_POINT)
            {
                FiffTag::read_tag(t_pStream.data(), t_pTag, pos);
                dig.append(t_pTag->toDigPoint());
            }
            else
            {
                if (kind == FIFF_MNE_COORD_FRAME)
                {
                    FiffTag::read_tag(t_pStream.data(), t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_MNE_COORD_FRAME" << t_pTag->getType();
                    coord_frame = *t_pTag->toInt();
                }
                else if (kind == FIFF_COORD_TRANS)
                {
                    FiffTag::read_tag(t_pStream.data(), t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_COORD_TRANS" << t_pTag->getType();
                    dig_trans = t_pTag->toCoordTrans();
                }
            }
        }
    }
    for(k = 0; k < dig.size(); ++k){
        dig[k].coord_frame = coord_frame;
    }

    //
    //   All kinds of auxliary stuff
    //

    t_pStream->device()->close();
    return dig;
}
