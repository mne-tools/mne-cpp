//=============================================================================================================
/**
 * @file     layoutloader.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    LayoutLoader class declaration.
 *
 */

#ifndef LAYOUTLOADER_H
#define LAYOUTLOADER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <string>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QStringList>
#include <QString>
#include <QPoint>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// DEFINES
//=============================================================================================================

//=============================================================================================================
/**
 * Processes layout files (AsA .elc, MNE .lout) files which contain the electrode positions of a EEG/MEG hat.
 *
 * @brief Processes AsA .elc files which contain the electrode positions of a EEG hat.
 */
class UTILSSHARED_EXPORT LayoutLoader
{
public:
    typedef QSharedPointer<LayoutLoader> SPtr;            /**< Shared pointer type for LayoutLoader. */
    typedef QSharedPointer<const LayoutLoader> ConstSPtr; /**< Const shared pointer type for LayoutLoader. */

    //=========================================================================================================
    /**
     * Reads the specified ANT elc-layout file.
     * @param[in] path holds the file path of the elc file which is to be read.
     * @param[in] location3D holds the vector to which the read 3D positions are stored.
     * @param[in] location2D holds the vector to which the read 2D positions are stored.
     * @return true if reading was successful, false otherwise.
     */
    static bool readAsaElcFile(const QString &path,
                               QStringList &channelNames,
                               QList<QVector<float> > &location3D,
                               QList<QVector<float> > &location2D,
                               QString &unit);

    //=========================================================================================================
    /**
     * Reads the specified ANT elc-layout file.
     * @param[in] path holds the file path of the elc file which is to be read.
     * @param[in] location3D holds the vector to which the read 3D positions are stored.
     * @param[in] location2D holds the vector to which the read 2D positions are stored.
     * @return true if reading was successful, false otherwise.
     */
    static bool readAsaElcFile(const std::string &path,
                               std::vector<std::string> &channelNames,
                               std::vector<std::vector<float> > &location3D,
                               std::vector<std::vector<float> > &location2D,
                               std::string &unit);

    //=========================================================================================================
    /**
     * Reads the specified MNE .lout file.
     * @param[in] path holds the file path of the lout file which is to be read.
     * @param[in] channel data holds the x,y and channel number for every channel. The map keys are the channel names (i.e. 'MEG 0113').
     * @return bool true if reading was successful, false otherwise.
     */
    static bool readMNELoutFile(const QString &path,
                                QMap<QString, QPointF> &channelData);

    //=========================================================================================================
    /**
     * Reads the specified MNE .lout file.
     * @param[in] path holds the file path of the lout file which is to be read.
     * @param[in] channel data holds the x,y and channel number for every channel. The map keys are the channel names (i.e. 'MEG 0113').
     * @return bool true if reading was successful, false otherwise.
     */
    static bool readMNELoutFile(const std::string &path,
                                QMap<std::string, QPointF> &channelData);

private:
};
} // NAMESPACE

#endif // LAYOUTLOADER_H
