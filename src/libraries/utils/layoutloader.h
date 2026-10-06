//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     layoutloader.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2014
 * @brief    Reader for ANT @c .elc electrode files used by the topographic plotting widgets.
 *
 * ANT @c .elc is the ASCII electrode list of ANT/eemagine systems with 3-D head-frame
 * and 2-D projected positions and a unit declaration (mm / cm / m). MNE-C @c .lout
 * layouts are read by @ref MNELIB::MNELayout.
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
 * Reads ANT .elc files, which contain the electrode positions of an EEG cap.
 *
 * @brief Reads ANT .elc electrode files into Qt/STL containers.
 *
 * @snippet ex_utils/main.cpp layout_loader_usage
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
     * @param[in, out] channelNames Receives the channel names.
     * @param[in] location3D holds the vector to which the read 3D positions are stored.
     * @param[in] location2D holds the vector to which the read 2D positions are stored.
     * @param[in, out] unit Receives the unit of the positions.
     * @return true if reading was successful, false otherwise.
     */
    static bool readAsaElcFile(const QString& path,
                               QStringList& channelNames,
                               QList<QVector<float>>& location3D,
                               QList<QVector<float>>& location2D,
                               QString& unit);

    //=========================================================================================================
    /**
     * Reads the specified ANT elc-layout file.
     * @param[in] path holds the file path of the elc file which is to be read.
     * @param[in, out] channelNames Receives the channel names.
     * @param[in] location3D holds the vector to which the read 3D positions are stored.
     * @param[in] location2D holds the vector to which the read 2D positions are stored.
     * @param[in, out] unit Receives the unit of the positions.
     * @return true if reading was successful, false otherwise.
     */
    static bool readAsaElcFile(const std::string& path,
                               std::vector<std::string>& channelNames,
                               std::vector<std::vector<float>>& location3D,
                               std::vector<std::vector<float>>& location2D,
                               std::string& unit);

private:
};
} // NAMESPACE

#endif // LAYOUTLOADER_H
