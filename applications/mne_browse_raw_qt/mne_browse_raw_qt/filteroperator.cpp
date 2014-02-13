//=============================================================================================================
/**
* @file     filteroperator.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     February, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains all FilterOperators.
*
*/


#include "filteroperator.h"
#include "mneoperator.h"

//*************************************************************************************************************

FilterOperator::FilterOperator()
: MNEOperator()
{

}

//*************************************************************************************************************

/**
* @brief FilterOperator::FilterOperator
* @param type of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType)
* @param order represents the order of the filter, the higher the higher is the stopband attenuation
* @param centerfreq determines the center of the frequency
* @param bandwidth ignored if FilterType is set to LPF,HPF. if NOTCH/BPF: bandwidth of stop-/passband
* @param parkswidth determines the width of the filter slopes (steepness)
*/
FilterOperator::FilterOperator(FilterType type, qint8 order, double centerfreq, double bandwidth, double parkswidth, qint32 fftlength)
: MNEOperator()
, m_iFilterOrder(order)
, m_Type(type)
, m_iFFTlength(fftlength)
{
    ParksMcClellan filter(order, centerfreq, bandwidth, parkswidth, type);
    RowVectorXd t_coeffs = filter.FirCoeff; //ToDo: change output datatype to RowVectorXd

    //zero-padding
}

