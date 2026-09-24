//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     info.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2015
 * @brief    Contains general application information like: application name and version number.
 */
#ifndef INFO_H
#define INFO_H

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QString>
#include <QObject>

//=============================================================================================================
// DEFINE NAMESPACE MNEANALYZE
//=============================================================================================================

namespace MNEANALYZE
{

//=============================================================================================================
/**
 * Log level
 */
enum LogLevel
{
    _LogLvMin,      /**< Minimal log information. */
    _LogLvNormal,   /**< Normal amount of log information. */
    _LogLvMax       /**< Accurate logging. */
};

//=============================================================================================================
/**
 * Log kind
 */
enum LogKind
{
    _LogKndMessage,     /**< Normal log message. */
    _LogKndWarning,     /**< Warning log message. */
    _LogKndError        /**< Error log message. */
};

//=============================================================================================================
/**
 * DECLARE CLASS CInfo
 *
 * @brief The CInfo class provides application information.
 */
class CInfo
{
public:
    //=========================================================================================================
    /**
     * Returns the short form of the application name.
     *
     * @return a string containing the short application name.
     */
    const static QString AppNameShort()
    {
        return QObject::tr("MNE Analyze");
    }

    //=========================================================================================================
    /**
     * Returns the application name.
     *
     * @return a string containing application name.
     */
    const static QString AppName()
    {
        return QObject::tr("Browse, Process & Visualize.");
    }

    //=========================================================================================================
    /**
     *
     */
    const static QString OrganizationName()
    {
        return QObject::tr("MNE-CPP");
    }

    //=========================================================================================================
    /**
     * Returns the major version number of the application which indicates larger changes of the application.
     *
     * @return the major version number.
     */
    static int MajorVersion()
    {
        return 1;
    }

    //=========================================================================================================
    /**
     * Returns the minor version number of the application which indicates smaller changes of the application.
     *
     * @return the minor version number.
     */
    static int MinorVersion()
    {
        return 0;
    }

    //=========================================================================================================
    /**
     * Returns the revision number which indicates the bug fix level.
     *
     * @return the revision number.
     */
    static int RevisionVersion()
    {
        return 0;
    }

    //=========================================================================================================
    /**
     * Returns the build number which corresponds to the SVN revision control number.
     *
     * @return the build number.
     */
    static int BuildVersion()
    {
        return 0;
    }

    //=========================================================================================================
    /**
     * Returns the version number (major.minor.build-revision) of the application.
     *
     * @return the full version number.
     */
    const static QString AppVersion()
    {
        //return QString("%1.%2.%3-%4").arg(MajorVersion()).arg(MinorVersion()).arg(RevisionVersion()).arg(BuildVersion());
        return QString("%1.%2.%3").arg(MajorVersion()).arg(MinorVersion()).arg(RevisionVersion());
    }
};

} //NAMESPACE

#endif // INFO_H
