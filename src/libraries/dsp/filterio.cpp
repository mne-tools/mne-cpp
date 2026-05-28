//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file filterio.cpp
 * @since 2026
 * @date  March 2026
 * @brief Definition of the FilterIO class
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

using namespace UTILSLIB;
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
