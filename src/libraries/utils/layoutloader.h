//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     layoutloader.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2014
 * @brief    Reader for ANT @c .elc electrode files and MNE @c .lout 2-D channel layouts used by the topographic plotting widgets.
 *
 * @ref UTILSLIB::LayoutLoader parses two complementary layout
 * descriptions that mne-cpp consumes throughout its UI and
 * forward-modelling stack:
 *
 * - ANT @c .elc — the ASCII electrode list emitted by
 *   ANT/eemagine systems, carrying both 3-D head-frame and
 *   2-D projected positions plus a per-file unit declaration
 *   (mm / cm / m); used when a user imports a new EEG cap or
 *   when DISP3DLIB builds the sensor visualisation.
 *
 * - MNE @c .lout — the legacy MNE-C topographic layout used
 *   by @c DISPLIB::ButterflyView and the time-frequency plots,
 *   storing per-channel @c (x,y,width,height,name) tuples.
 *
 * Both @c QString and @c std::string overloads are provided so
 * the loader is callable from the Qt-aware GUI layer and from
 * the pure-C++ test fixtures without a string conversion.
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
 * @brief Reads ANT .elc electrode files and MNE .lout 2-D channel layouts into Qt/STL containers.
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
