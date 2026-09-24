//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     info.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     January, 2014
 * @version  2.1.0
 * @brief    Contains general application information like: application name and version number.
 */

#ifndef INFO_H
#define INFO_H

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QString>
#include <QObject>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * Log level
 */
enum LogLevel
{
    _LogLvMin,      /**< Minimal log information */
    _LogLvNormal,   /**< Normal amount of log information */
    _LogLvMax       /**< Accurate logging */
};


//=============================================================================================================
/**
 * Log kind
 */
enum LogKind
{
    _LogKndMessage,     /**< Normal log message */
    _LogKndWarning,     /**< Warning log message */
    _LogKndError        /**< Error log message */
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
        return QObject::tr("mne_analyze_studio_fiff_browser");
    }

    //=========================================================================================================
    /**
     * Returns the application name.
     *
     * @return a string containing application name.
     */
    const static QString AppName()
    {
        return QObject::tr("Embedded FIFF raw browser for MNE Analyze Studio.");
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
        return 2;
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
        return 224;
    }

    //=========================================================================================================
    /**
     * Returns the version number (major.minor.build-revision) of the application.
     *
     * @return the full version number.
     */
    const static QString AppVersion()
    {
        return QString("%1.%2.%3-%4").arg(MajorVersion()).arg(MinorVersion()).arg(RevisionVersion()).arg(BuildVersion());
    }
};

} //NAMESPACE

#endif // INFO_H
