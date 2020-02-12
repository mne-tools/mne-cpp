//=============================================================================================================
/**
 * @file     filterio.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     April, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FilterIO class
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterio.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterIO::FilterIO()
{
}


//*************************************************************************************************************

bool FilterIO::readFilter(QString path, FilterData &filter)
{
    //Open .txt file
    if(!path.contains(".txt"))
        return false;

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening filter txt file for reading";
        return false;
    }

    //Start reading from file
    QTextStream in(&file);
    QVector<double> coefficientsTemp;

    while(!in.atEnd())
    {
        QString line = in.readLine();

        QStringList fields = line.split(QRegExp("\\s+"));

        //Delete last element if it is a blank character
        if(fields.at(fields.size()-1) == "")
            fields.removeLast();

        if(line.contains("#")) //Filter meta information commented areas in file
        {
            //Read filter sFreq
            if(line.contains("sFreq") && fields.size()==2)
                filter.m_sFreq = fields.at(1).toDouble();

            //Read filter name
            if(line.contains("name")) {
                filter.m_sName.clear();
                for(int i=1; i<fields.size(); i++)
                    filter.m_sName.append(fields.at(i));
            }

            //Read the filter order
            if(line.contains("order") && fields.size()==2)
                filter.m_iFilterOrder = fields.at(1).toInt();

            //Read the filter type
            if(line.contains("type") && fields.size()==2)
                filter.m_Type = FilterData::getFilterTypeForString(fields.at(1));

            //Read the filter LPFreq
            if(line.contains("LPFreq") && fields.size()==2)
                filter.m_dLowpassFreq = fields.at(1).toDouble();

            //Read the filter HPFreq
            if(line.contains("HPFreq") && fields.size()==2)
                filter.m_dHighpassFreq = fields.at(1).toDouble();

            //Read the filter CenterFreq
            if(line.contains("CenterFreq") && fields.size()==2)
                filter.m_dCenterFreq = fields.at(1).toDouble();

            //Read the filter DesignMethod
            if(line.contains("DesignMethod") && fields.size()==2)
                filter.m_designMethod = FilterData::getDesignMethodForString(fields.at(1));

        } else // Read filter coefficients
            coefficientsTemp.push_back(fields.join("").toDouble());
    }
    // Check if reading was successful and correct
    if(filter.m_iFilterOrder != coefficientsTemp.size())
        filter.m_iFilterOrder = coefficientsTemp.size();

//    if(filter.m_sFreq)

//    if(filter.m_sName)

//    if(filter.m_Type)

//    if(filter.m_dLowpassFreq)

//    if(filter.m_dHighFreq)

//    if(filter.m_designMethod)

    filter.m_dCoeffA = RowVectorXd::Zero(coefficientsTemp.size());
    for(int i=0; i<filter.m_dCoeffA.cols(); i++)
        filter.m_dCoeffA(i) = coefficientsTemp.at(i);

    //Compute fft of filtercoeeficients
    filter.fftTransformCoeffs();

    file.close();

    return true;
}


//*************************************************************************************************************

bool FilterIO::writeFilter(const QString &path, const FilterData &filter)
{
    // Open file dialog
    if(!path.isEmpty())
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug()<<"Error opening filter txt file for writing";
            return false;
        }

        //Write coefficients to file
        QTextStream out(&file);

        out << "#sFreq " << filter.m_sFreq << "\n";
        out << "#name " << filter.m_sName << "\n";
        out << "#type " << FilterData::getStringForFilterType(filter.m_Type) << "\n";
        out << "#order " << filter.m_iFilterOrder << "\n";
        out << "#HPFreq " << filter.m_dHighpassFreq << "\n";
        out << "#LPFreq " << filter.m_dLowpassFreq << "\n";
        out << "#CenterFreq " << filter.m_dCenterFreq << "\n";
        out << "#DesignMethod " << FilterData::getStringForDesignMethod(filter.m_designMethod) << "\n";

        for(int i = 0 ; i<filter.m_dCoeffA.cols() ;i++)
            out << filter.m_dCoeffA(i) << "\n";

        file.close();

        return true;
    }

    qDebug()<<"Error Filter File path is empty";

    return false;
}
