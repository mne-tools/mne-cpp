//=============================================================================================================
/**
 * @file     ioutils.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
#include <ios>
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

struct UTILSSHARED_EXPORT aaaa
{
    int i = 9;
};

struct UTILSSHARED_EXPORT b
{
    int i = 4;
};

//=============================================================================================================
/**
 * IO utilitie routines
 *
 * @brief IO utilitie routines
 */
class UTILSSHARED_EXPORT IOUtils
{
public:
    typedef QSharedPointer<IOUtils> SPtr;            /**< Shared pointer type for IOUtils class. */
    typedef QSharedPointer<const IOUtils> ConstSPtr; /**< Const shared pointer type for IOUtils class. */

    //=========================================================================================================
    /**
     * Destroys the IOUtils class.
     */
    ~IOUtils(){};

    //=========================================================================================================
    /**
     * mne_fread3(fid)
     *
     * Reads a 3-byte integer out of a stream
     *
     * @param[in] p_qStream  Stream to read from.
     *
     * @return the read 3-byte integer.
     */
    static qint32 fread3(QDataStream &p_qStream);

    //=========================================================================================================
    /**
     * mne_fread3(fid)
     *
     * Reads a 3-byte integer out of a stream
     *
     * @param[in] stream  Stream to read from.
     *
     * @return the read 3-byte integer.
     */
    static qint32 fread3(std::iostream& stream);

    //=========================================================================================================
    /**
     * fread3_many(fid,count)
     *
     * Reads a 3-byte integer out of a stream
     *
     * @param[in] p_qStream  Stream to read from.
     * @param[in] count      Number of elements to read.
     *
     * @return the read 3-byte integer.
     */
    static Eigen::VectorXi fread3_many(QDataStream &p_qStream, qint32 count);

    //=========================================================================================================
    /**
     * fread3_many(fid,count)
     *
     * Reads a 3-byte integer out of a stream
     *
     * @param[in] stream     Stream to read from.
     * @param[in] count      Number of elements to read.
     *
     * @return the read 3-byte integer.
     */
    static Eigen::VectorXi fread3_many(std::iostream &stream, qint32 count);

    //=========================================================================================================
    /**
     * swap short
     *
     * @param[in] source     short to swap.
     *
     * @return swapped short.
     */
    static qint16 swap_short (qint16 source);

    //=========================================================================================================
    /**
     * swap integer
     *
     * @param[in] source     integer to swap.
     *
     * @return swapped integer.
     */
    static qint32 swap_int (qint32 source);

    //=========================================================================================================
    /**
     * swap integer
     *
     * @param[in, out] source     integer to swap.
     */
    static void swap_intp (qint32 *source);

    //=========================================================================================================
    /**
     * swap long
     *
     * @param[in] source     long to swap.
     *
     * @return swapped long.
     */
    static qint64 swap_long (qint64 source);

    //=========================================================================================================
    /**
     * swap long
     *
     * @param[in, out] source     long to swap.
     */
    static void swap_longp (qint64 *source);

    //=========================================================================================================
    /**
     * swap float
     *
     * @param[in] source     float to swap.
     *
     * @return swapped float.
     */
    static float swap_float (float source);

    //=========================================================================================================
    /**
     * swap float
     *
     * @param[in, out] source     float to swap.
     */
    static void swap_floatp (float *source);

    //=========================================================================================================
    /**
     * swap double
     *
     * @param[in, out] source     double to swap.
     */
    static void swap_doublep(double *source);

    //=========================================================================================================
    /**
     * Write Eigen Matrix to file
     *
     * @param[in] in         input eigen value which is to be written to file.
     * @param[in] path       path and file name to write to.
     */
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& in, const QString& sPath, const QString& sDescription = QString());
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, 1, Eigen::Dynamic>& in, const QString& sPath, const QString& sDescription = QString());
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, 1>& in, const QString& sPath, const QString& sDescription = QString());

    //=========================================================================================================
    /**
     * Write Eigen Matrix to file
     *
     * @param[in] in         input eigen value which is to be written to file.
     * @param[in] path       path and file name to write to.
     */
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& in, const std::string& sPath, const std::string& sDescription = std::string());
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, 1, Eigen::Dynamic>& in, const std::string& sPath, const std::string& sDescription = std::string());
    template<typename T>
    static bool write_eigen_matrix(const Eigen::Matrix<T, Eigen::Dynamic, 1>& in, const std::string& sPath, const std::string& sDescription = std::string());

    //=========================================================================================================
    /**
     * Read Eigen Matrix from file
     *
     * @param[out] out       output eigen value.
     * @param[in] path       path and file name to read from.
     */
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& out, const QString& path);
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, 1, Eigen::Dynamic>& out, const QString& path);
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, 1>& out, const QString& path);

    //=========================================================================================================
    /**
     * Read Eigen Matrix from file
     *
     * @param[out] out       output eigen value.
     * @param[in] path       path and file name to read from.
     */
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& out, const std::string& path);
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, 1, Eigen::Dynamic>& out, const std::string& path);
    template<typename T>
    static bool read_eigen_matrix(Eigen::Matrix<T, Eigen::Dynamic, 1>& out, const std::string& path);

    //=========================================================================================================
    /**
     * Returns the new channel naming conventions (whitespcae between channel type and number) for the input list.
     *
     * @param[in] chNames    The channel names.
     *
     * @return The new channel names.
     */
    static QStringList get_new_chnames_conventions(const QStringList& chNames);

    //=========================================================================================================
    /**
     * Returns the new channel naming conventions (whitespcae between channel type and number) for the input list.
     *
     * @param[in] chNames    The channel names.
     *
     * @return The new channel names.
     */
    static std::vector<std::string> get_new_chnames_conventions(const std::vector<std::string>& chNames);

    //=========================================================================================================
    /**
     * Returns the old channel naming conventions (whitespcae between channel type and number) for the input list.
     *
     * @param[in] chNames    The channel names.
     *
     * @return The new channel names.
     */
    static QStringList get_old_chnames_conventions(const QStringList& chNames);

    //=========================================================================================================
    /**
     * Returns the old channel naming conventions (whitespcae between channel type and number) for the input list.
     *
     * @param[in] chNames    The channel names.
     *
     * @return The new channel names.
     */
    static std::vector<std::string> get_old_chnames_conventions(const std::vector<std::string>& chNames);

    //=========================================================================================================
    /**
     * Checks if all names from chNamesA are in chNamesB. If wanted each name in chNamesA is transformed to the old and new naming convention and checked if in chNamesB.
     *
     * @param[in] chNamesA    The channel names.
     * @param[in] chNamesB    The channel names which is to be compared to.
     * @param[in] bCheckForNewNamingConvention    Whether to use old and new naming conventions while checking.
     *
     * @return True if all names in chNamesA are present in chNamesB, false otherwise.
     */
    static bool check_matching_chnames_conventions(const QStringList& chNamesA, const QStringList& chNamesB, bool bCheckForNewNamingConvention = false);

    //=========================================================================================================
    /**
     * Checks if all names from chNamesA are in chNamesB. If wanted each name in chNamesA is transformed to the old and new naming convention and checked if in chNamesB.
     *
     * @param[in] chNamesA    The channel names.
     * @param[in] chNamesB    The channel names which is to be compared to.
     * @param[in] bCheckForNewNamingConvention    Whether to use old and new naming conventions while checking.
     *
     * @return True if all names in chNamesA are present in chNamesB, false otherwise.
     */
    static bool check_matching_chnames_conventions(const std::vector<std::string>& chNamesA, const std::vector<std::string>& chNamesB, bool bCheckForNewNamingConvention = false);
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
                QStringList fields = line.split(QRegExp("\\s+"));

                //Delete last element if it is a blank character
                if(fields.at(fields.size()-1) == "")
                    fields.removeLast();

                Eigen::VectorXd x (fields.size());

                for (int j=0; j<fields.size(); j++) {
                    x(j)=fields.at(j).toDouble();
                }

                help.append(x);
            }
        }

        int rows = help.size();
        int cols = rows<=0 ? 0 : help.at(0).rows();

        out.resize(rows, cols);

        for (int i=0; i<help.length(); i++) {
            out.row(i)=help[i].transpose();
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
        size_t matSize{0};

        while(std::getline(inputFile, line)){
            if(std::find(line.begin(), line.end(), '#') == line.end()){
                std::vector<double> elements;
                std::stringstream stream{line};
                std::string element;

                stream >> std::ws;
                while(stream >> element){
                    elements.push_back(std::stod(element));
                }

                Eigen::VectorXd x (elements.size());

                for(int i = 0; i < elements.size(); ++i){
                    x(i) = elements.at(i);
                }

                help.push_back(std::move(x));
            }
        }

        int rows = help.size();
        int cols = rows<=0 ? 0 : help.at(0).rows();

        out.resize(rows, cols);

        for (int i=0; i < help.size(); i++) {
            out.row(i)=help[i].transpose();
        }
    } else {
        qWarning()<<"IOUtils::read_eigen_matrix - Could not read Eigen element from file! Path does not exist!";
        return false;
    }

    return true;
}
} // NAMESPACE

#endif // IOUTILS_H

