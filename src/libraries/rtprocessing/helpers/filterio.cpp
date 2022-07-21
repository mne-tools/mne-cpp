//=============================================================================================================
/**
 * @file     filterio.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterio.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QRegularExpression>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterIO::FilterIO()
{
}

//=============================================================================================================

bool FilterIO::readFilter(QString path, FilterKernel &filter)
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

        QStringList fields = line.split(QRegularExpression("\\s+"));

        //Delete last element if it is a blank character
        if(fields.at(fields.size()-1) == "")
            fields.removeLast();

        if(line.contains("#")) //Filter meta information commented areas in file
        {
            //Read filter sFreq
            if(line.contains("sFreq") && fields.size()==2)
                filter.setSamplingFrequency(fields.at(1).toDouble());

            //Read filter name
            if(line.contains("name")) {
                QString sFilterName;
                for(int i=1; i<fields.size(); i++)
                    sFilterName.append(fields.at(i));
                filter.setName(sFilterName);
            }

            //Read the filter order
            if(line.contains("order") && fields.size()==2)
                filter.setFilterOrder(fields.at(1).toInt());

            //Read the filter type
            if(line.contains("type") && fields.size()==2)
                filter.setFilterType(FilterKernel::m_filterTypes.indexOf(FilterParameter(fields.at(1))));

            //Read the filter LPFreq
            if(line.contains("LPFreq") && fields.size()==2)
                filter.setLowpassFreq(fields.at(1).toDouble());

            //Read the filter HPFreq
            if(line.contains("HPFreq") && fields.size()==2)
                filter.setHighpassFreq(fields.at(1).toDouble());

            //Read the filter CenterFreq
            if(line.contains("CenterFreq") && fields.size()==2)
                filter.setCenterFrequency(fields.at(1).toDouble());

            //Read the filter DesignMethod
            if(line.contains("DesignMethod") && fields.size()==2)
                filter.setDesignMethod(FilterKernel::m_designMethods.indexOf(FilterParameter(fields.at(1))));

        } else // Read filter coefficients
            coefficientsTemp.push_back(fields.join("").toDouble());
    }
    // Check if reading was successful and correct
    if(filter.getFilterOrder() != coefficientsTemp.size()) {
        filter.setFilterOrder(coefficientsTemp.size());
    }

    RowVectorXd vecCoeff = RowVectorXd::Zero(coefficientsTemp.size());
    for(int i=0; i < vecCoeff.cols(); i++) {
        vecCoeff(i) = coefficientsTemp.at(i);
    }
    filter.setCoefficients(vecCoeff);

    file.close();

    return true;
}

//=============================================================================================================

bool FilterIO::writeFilter(const QString &path, const FilterKernel &filter)
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

        out << "#sFreq " << filter.getSamplingFrequency() << "\n";
        out << "#name " << filter.getName() << "\n";
        out << "#type " << filter.getFilterType().getName() << "\n";
        out << "#order " << filter.getFilterOrder() << "\n";
        out << "#HPFreq " << filter.getHighpassFreq() << "\n";
        out << "#LPFreq " << filter.getLowpassFreq() << "\n";
        out << "#CenterFreq " << filter.getCenterFrequency() << "\n";
        out << "#DesignMethod " << filter.getDesignMethod().getName() << "\n";

        for(int i = 0 ; i<filter.getCoefficients().cols() ;i++)
            out << filter.getCoefficients()(i) << "\n";

        file.close();

        return true;
    }

    qDebug()<<"Error Filter File path is empty";

    return false;
}
