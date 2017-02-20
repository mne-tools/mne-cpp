//=============================================================================================================
/**
* @file     fiff_dig_point_set.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Jana Kiesel and Matti Hamalainen. All rights reserved.
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
* @brief    fiff_dig_point_set class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_point_set.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_point.h"
#include "fiff_dir_node.h"
#include "fiff_tag.h"
#include "fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDigPointSet::FiffDigPointSet()
    :m_qListDigPoint()
{
}


//*************************************************************************************************************

FiffDigPointSet::FiffDigPointSet(const FiffDigPointSet &p_FiffDigPointSet)
: m_qListDigPoint(p_FiffDigPointSet.m_qListDigPoint)
{

}


//*************************************************************************************************************

FiffDigPointSet::FiffDigPointSet(QIODevice &p_IODevice)   //const FiffDigPointSet &p_FiffDigPointSet
{
    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));

    if(!FiffDigPointSet::readFromStream(t_pStream, *this))
    {
        t_pStream->close();
        qDebug() << "Could not read the FiffDigPointSet\n"; // ToDo throw error
    }
    qDebug("%i digitizer Points read in file.", this->size());
}


//*************************************************************************************************************

FiffDigPointSet::~FiffDigPointSet()
{

}


//*************************************************************************************************************

bool FiffDigPointSet::readFromStream(FiffStream::SPtr &p_Stream, FiffDigPointSet &p_Dig)
{
    //
    //   Open the file, create directory
    //
    bool open_here = false;

    if (!p_Stream->device()->isOpen()) {
        QString t_sFileName = p_Stream->streamName();

        if(!p_Stream->open())
            return false;

        printf("Opening header data %s...\n",t_sFileName.toUtf8().constData());

        open_here = true;
    }

    //
    //   Read the measurement info
    //
    //read_hpi_info(p_pStream,p_Tree, info);
    fiff_int_t kind = -1;
    fiff_int_t pos = -1;
    FiffTag::SPtr t_pTag;

    //
    //   Locate the Electrodes
    //
    QList<FiffDirNode::SPtr> isotrak = p_Stream->tree()->dir_tree_find(FIFFB_ISOTRAK);

    fiff_int_t coord_frame = FIFFV_COORD_HEAD;
    FiffCoordTrans dig_trans;
    qint32 k = 0;

    if (isotrak.size() == 1)
    {
        for (k = 0; k < isotrak[0]->nent; ++k)
        {
            kind = isotrak[0]->dir[k]->kind;
            pos  = isotrak[0]->dir[k]->pos;
            if (kind == FIFF_DIG_POINT)
            {
                FiffTag::read_tag(p_Stream, t_pTag, pos);
                p_Dig.m_qListDigPoint.append(t_pTag->toDigPoint());
            }
            else
            {
                if (kind == FIFF_MNE_COORD_FRAME)
                {
                    FiffTag::read_tag(p_Stream, t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_MNE_COORD_FRAME" << t_pTag->getType();
                    coord_frame = *t_pTag->toInt();
                }
                else if (kind == FIFF_COORD_TRANS)
                {
                    FiffTag::read_tag(p_Stream, t_pTag, pos);
                    qDebug() << "NEEDS To BE DEBBUGED: FIFF_COORD_TRANS" << t_pTag->getType();
                    dig_trans = t_pTag->toCoordTrans();
                }
            }
        }
    }
    for(k = 0; k < p_Dig.size(); ++k)
    {
        p_Dig[k].coord_frame = coord_frame;
    }

    //
    //   All kinds of auxliary stuff
    //
    if(open_here)
    {
        p_Stream->close();
    }
    return true;
}


//*************************************************************************************************************

void FiffDigPointSet::write(QIODevice &p_IODevice)
{
    //
    //   Open the file, create directory
    //

    // Create the file and save the essentials
    FiffStream::SPtr t_pStream = FiffStream::start_file(p_IODevice);
    printf("Write Digitizer Points in %s...\n", t_pStream->streamName().toUtf8().constData());
    this->writeToStream(t_pStream.data());
}


//*************************************************************************************************************

void FiffDigPointSet::writeToStream(FiffStream* p_pStream)
{
    p_pStream->start_block(FIFFB_MEAS);
    p_pStream->start_block(FIFFB_MEAS_INFO);
    p_pStream->start_block(FIFFB_ISOTRAK);

    for(qint32 h = 0; h < m_qListDigPoint.size(); ++h)
    {
        p_pStream->write_dig_point(m_qListDigPoint[h]);
    }

    printf("\t%d digitizer points written\n", m_qListDigPoint.size());
    p_pStream->end_block(FIFFB_ISOTRAK);
    p_pStream->end_block(FIFFB_MEAS_INFO);
    p_pStream->end_block(FIFFB_MEAS);
    p_pStream->end_file();
}

//*************************************************************************************************************

const FiffDigPoint& FiffDigPointSet::operator[] (qint32 idx) const
{
    if (idx>=m_qListDigPoint.length())
    {
        qWarning("Warning: Required DigPoint doesn't exist! Returning DigPoint '0'.");
        idx=0;
    }
    return m_qListDigPoint[idx];
}


//*************************************************************************************************************

FiffDigPoint& FiffDigPointSet::operator[] (qint32 idx)
{
    if (idx >= m_qListDigPoint.length())
    {
        qWarning("Warning: Required DigPoint doesn't exist! Returning DigPoint '0'.");
        idx = 0;
    }
    return m_qListDigPoint[idx];
}


//*************************************************************************************************************

FiffDigPointSet &FiffDigPointSet::operator<<(const FiffDigPoint &dig)
{
    this->m_qListDigPoint.append(dig);
    return *this;
}


//*************************************************************************************************************

FiffDigPointSet &FiffDigPointSet::operator<<(const FiffDigPoint *dig)
{
    this->m_qListDigPoint.append(*dig);
    return *this;
}
