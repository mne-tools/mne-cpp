//=============================================================================================================
/**
 * @file     ioutils.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief    IOUtils class declaration
 *
 */

#ifndef IOUTILS_H
#define IOUTILS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * IO utility routines for reading/writing Eigen matrices to text files.
 *
 * @brief Eigen matrix I/O utilities.
 */
class UTILSSHARED_EXPORT IOUtils
{
public:
    typedef QSharedPointer<IOUtils> SPtr;            /**< Shared pointer type for IOUtils class. */
    typedef QSharedPointer<const IOUtils> ConstSPtr; /**< Const shared pointer type for IOUtils class. */

    ~IOUtils(){};

    //=========================================================================================================
    /**
     * Write Eigen Matrix to file (QString path).
     */
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& in, const QString& sPath, const QString& sDescription = QString());
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, 1, Eigen::Dynamic>& in, const QString& sPath, const QString& sDescription = QString());
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, 1>& in, const QString& sPath, const QString& sDescription = QString());

    //=========================================================================================================
    /**
     * Write Eigen Matrix to file (std::string path).
     */
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& in, const std::string& sPath, const std::string& sDescription = std::string());
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, 1, Eigen::Dynamic>& in, const std::string& sPath, const std::string& sDescription = std::string());
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, 1>& in, const std::string& sPath, const std::string& sDescription = std::string());

    //=========================================================================================================
    /**
     * Read Eigen Matrix from file (QString path).
     */
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& out, const QString& path);
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, 1, Eigen::Dynamic>& out, const QString& path);
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, 1>& out, const QString& path);

    //=========================================================================================================
    /**
     * Read Eigen Matrix from file (std::string path).
     */
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& out, const std::string& path);
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, 1, Eigen::Dynamic>& out, const std::string& path);
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, 1>& out, const std::string& path);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

template<typename T>
bool IOUtils::write_eigen_matrix(const Eigen::Matrix<T, 1, Eigen::Dynamic>& in, const QString& sPath, const QString& sDescription)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(1,in.cols());
    matrixName.row(0)= in;
    return IOUtils::write_eigen_matrix(matrixName, sPath, sDescription);
}

//=============================================================================================================

template<typename T>
bool IOUtils::write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, 1>& in, const QString& sPath, const QString& sDescription)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(in.rows(),1);
    matrixName.col(0)= in;
    return IOUtils::write_eigen_matrix(matrixName, sPath, sDescription);
}

//=============================================================================================================

template<typename T>
bool IOUtils::write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& in, const QString& sPath, const QString& sDescription)
{
    QFile file(sPath);
    if(file.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        QTextStream stream(&file);
        if(!sDescription.isEmpty()) {
            stream<<"# Dimensions (rows x cols): "<<in.rows()<<" x "<<in.cols()<<"\n";
            stream<<"# Description: "<<sDescription<<"\n";
        }

        for(int row = 0; row<in.rows(); row++) {
            for(int col = 0; col<in.cols(); col++)
                stream << in(row, col)<<" ";
            stream<<"\n";
        }
    } else {
        qWarning()<<"Could not write Eigen element to file! Path does not exist!";
        return false;
    }

    file.close();

    return true;
}

//=============================================================================================================

template<typename T>
bool IOUtils::write_eigen_matrix(const Eigen::Matrix<T, 1, Eigen::Dynamic>& in, const std::string& sPath, const std::string& sDescription)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(1,in.cols());
    matrixName.row(0)= in;
    return IOUtils::write_eigen_matrix(matrixName, sPath, sDescription);
}

//=============================================================================================================

template<typename T>
bool IOUtils::write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, 1>& in, const std::string& sPath, const std::string& sDescription)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName(in.rows(),1);
    matrixName.col(0)= in;
    return IOUtils::write_eigen_matrix(matrixName, sPath, sDescription);
}

//=============================================================================================================

template<typename T>
bool IOUtils::write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& in, const std::string& sPath, const std::string& sDescription)
{
    std::ofstream outputFile(sPath);
    if(outputFile.is_open())
    {
        if(!sDescription.empty()) {
            outputFile<<"# Dimensions (rows x cols): "<<in.rows()<<" x "<<in.cols()<<"\n";
            outputFile<<"# Description: "<<sDescription<<"\n";
        }

        for(int row = 0; row<in.rows(); row++) {
            for(int col = 0; col<in.cols(); col++)
                outputFile << in(row, col)<<" ";
            outputFile<<"\n";
        }
    } else {
        qWarning()<<"Could not write Eigen element to file! Path does not exist!";
        return false;
    }

    return true;
}

//=============================================================================================================

template<typename T>
bool IOUtils::read_eigen_matrix(Eigen::Matrix<T, 1, Eigen::Dynamic>& out, const QString& path)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName;
    bool bStatus = IOUtils::read_eigen_matrix(matrixName, path);

    if(matrixName.rows() > 0)
    {
        out = matrixName.row(0);
    }

    return bStatus;
}

//=============================================================================================================

template<typename T>
bool IOUtils::read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, 1>& out, const QString& path)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName;
    bool bStatus = IOUtils::read_eigen_matrix(matrixName, path);

    if(matrixName.cols() > 0)
    {
        out = matrixName.col(0);
    }

    return bStatus;
}

//=============================================================================================================

template<typename T>
bool IOUtils::read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& out, const QString& path)
{
    QFile file(path);

    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //Start reading from file
        QTextStream in(&file);
        QList<Eigen::VectorXd> help;

        while(!in.atEnd())
        {
            QString line = in.readLine();
            if(!line.contains("#")) {
                QStringList fields = line.split(QRegularExpression("\\s+"));

                //Delete last element if it is a blank character
                if(fields.at(fields.size()-1) == "")
                    fields.removeLast();

                Eigen::VectorXd x (fields.size());

                for (int j = 0; j<fields.size(); j++) {
                    x(j) = fields.at(j).toDouble();
                }

                help.append(x);
            }
        }

        int rows = help.size();
        int cols = rows <= 0 ? 0 : help.at(0).rows();

        out.resize(rows, cols);

        for (int i=0; i < help.length(); i++) {
            out.row(i) = help[i].transpose();
        }
    } else {
        qWarning()<<"IOUtils::read_eigen_matrix - Could not read Eigen element from file! Path does not exist!";
        return false;
    }

    return true;
}

//=============================================================================================================

template<typename T>
bool IOUtils::read_eigen_matrix(Eigen::Matrix<T, 1, Eigen::Dynamic>& out, const std::string& path)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName;
    bool bStatus = IOUtils::read_eigen_matrix(matrixName, path);

    if(matrixName.rows() > 0)
    {
        out = matrixName.row(0);
    }

    return bStatus;
}

//=============================================================================================================

template<typename T>
bool IOUtils::read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, 1>& out, const std::string& path)
{
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrixName;
    bool bStatus = IOUtils::read_eigen_matrix(matrixName, path);

    if(matrixName.cols() > 0)
    {
        out = matrixName.col(0);
    }

    return bStatus;
}

//=============================================================================================================

template<typename T>
bool IOUtils::read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& out, const std::string& path)
{
    std::ifstream inputFile(path);

    if(inputFile.is_open()) {
        //Start reading from file
        std::vector<Eigen::VectorXd> help;

        std::string line;

        while(std::getline(inputFile, line)){
            if(line.find('#') == std::string::npos){
                std::vector<double> elements;
                std::stringstream stream{line};
                std::string element;

                stream >> std::ws;
                while(stream >> element){
                    elements.push_back(std::stod(element));
                    stream >> std::ws;
                }

                Eigen::VectorXd x (elements.size());

                for(size_t i = 0; i < elements.size(); ++i){
                    x(i) = elements.at(i);
                }

                help.push_back(std::move(x));
            }
        }

        int rows = help.size();
        int cols = rows <= 0 ? 0 : help.at(0).rows();

        out.resize(rows, cols);

        for (size_t i = 0; i < help.size(); i++) {
            out.row(i) = help[i].transpose();
        }
    } else {
        qWarning()<<"IOUtils::read_eigen_matrix - Could not read Eigen element from file! Path does not exist!";
        return false;
    }

    return true;
}
} // NAMESPACE

#endif // IOUTILS_H

