//=============================================================================================================
/**
* @file     slicedata.h
* @author   Carsten Boensel <carsten.boensel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*           Bruce Fischl <fischl@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2015
*
* @section  LICENSE
*
* Copyright (C) July, 2015 Carsten Boensel and Matti Hamalainen. All rights reserved.
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
* @brief     ScliceData class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "slicedata.h"
//#include <disp/imagesc.h>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
//using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SliceData::SliceData()
{

}

SliceData::SliceData(MatrixXd slice)
{
    setDataAsMatrix(slice);
    m_idx = 0; // first and only slice
}

SliceData::SliceData(MatrixXd slice, quint32 idx)
{
    setDataAsMatrix(slice);
    m_idx = idx;
}

//*************************************************************************************************************

inline MatrixXd SliceData::getDataAsMatrix()
{
    return m_slice;
}

//*************************************************************************************************************

inline void SliceData::setDataAsMatrix(MatrixXd slice)
{
    // eventually clean m_slice before
    m_slice = slice;
    m_dimX = m_slice.RowsAtCompileTime;
    m_dimY = m_slice.ColsAtCompileTime;
}

//*************************************************************************************************************

inline void SliceData::setSliceIdx(quint32 idx)
{
    m_idx = idx;
}

//*************************************************************************************************************

QImage SliceData::getSliceAsImage()
{
    qint32 i, j;
    QImage t_qImageData(m_dimX, m_dimY, QImage::Format_RGB32);

    for(i = 0; i < m_dimX; ++i)
        for(j = 0; j < m_dimY; ++j)
//            t_qImageData.setPixel(i, j, ColorMap::valueToJet(m_slice(j,i)));
    //todo: check if this colomap function is used right for our kind of data.

    return t_qImageData;
    //todo: eventually port this functionality to imagesc and call it
}

//*************************************************************************************************************

inline double SliceData::getVoxel(int x, int y)
{
    return m_slice(x,y);
}

//*************************************************************************************************************

//void SliceData::show()
//{
//    ImageSc imagesc(m_slice);
//    QString title = "Visualization of slice no" + m_idx;
//    imagesc.setTitle(title.toStdString());
//    imagesc.setXLabel("X Axes");
//    imagesc.setYLabel("Y Axes");

//    imagesc.setColorMap("HotNeg2");
//    // Alternate color maps
//    //  imagesc.setColorMap("Jet");
//    //  imagesc.setColorMap("RedBlue");
//    //  imagesc.setColorMap("Bone");
//    //  imagesc.setColorMap("Jet");
//    //  imagesc.setColorMap("Hot");

//    imagesc.setWindowTitle("Slice Plot");
//    imagesc.show();
//}

//*************************************************************************************************************

SliceData::~SliceData()
{

}

