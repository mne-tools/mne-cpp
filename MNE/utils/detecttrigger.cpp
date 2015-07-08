//=============================================================================================================
/**
* @file     detecttrigger.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     July, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the DetectTrigger class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "detecttrigger.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DetectTrigger::DetectTrigger()
{

}


//*************************************************************************************************************

void DetectTrigger::detectTriggerFlanks(const MatrixXd &data, QMap<int,QList<int> >& qMapDetectedTrigger, int iOffsetIndex, double dThreshold)
{
    //TODO: This only can detect one trigger per data block. What iff there are more than one trigger in the data block?
    QMapIterator<int,QList<int> > i(qMapDetectedTrigger);
    while (i.hasNext()) {
        i.next();
        //detect the actual triggers in the current data matrix
        if(i.key() > data.rows()) {
//            std::cout<<i.key()<<std::endl;
//            std::cout<<"RealTimeMultiSampleArrayModel::detectTrigger - trigger channel index exceed matrix dimensions. Returning..."<<std::endl;
            return;
        }

        RowVectorXd stimSegment = data.row(i.key());
        RowVectorXd::Index indexMaxCoeff;
        int dMax = stimSegment.maxCoeff(&indexMaxCoeff);

        //Find trigger using gradient/difference
        double gradient = 0;

        if(indexMaxCoeff-10<0)
            gradient = stimSegment(indexMaxCoeff) - stimSegment(indexMaxCoeff+10);
        else
            gradient = stimSegment(indexMaxCoeff) - stimSegment(indexMaxCoeff-10);

//        std::cout<<"gradient: "<<gradient<<std::endl;
//        std::cout<<"indexMaxCoeff: "<<indexMaxCoeff<<std::endl;

        if(gradient>dThreshold) {
            qMapDetectedTrigger[i.key()].append((int)iOffsetIndex+indexMaxCoeff);
//            std::cout<<"m_iCurrentSample-data.cols()+indexMaxCoeff: "<<iOffsetIndex+indexMaxCoeff<<std::endl;
        }
    }
}


