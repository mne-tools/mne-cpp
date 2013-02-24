//=============================================================================================================
/**
* @file		info.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains general application information like: application name and  version number.
*
*/

#ifndef INFO_H
#define INFO_H


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QObject>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{


//*************************************************************************************************************
//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

//=============================================================================================================
/**
* Log level
*/
enum LogLevel
{
    _LogLvMin,		/**< Minimal log information */
    _LogLvNormal,	/**< Normal amount of log information */
    _LogLvMax		/**< Accurate logging */
};


//=============================================================================================================
/**
* Log kind
*/
enum LogKind
{
    _LogKndMessage,		/**< Normal log message */
    _LogKndWarning,		/**< Warning log message */
    _LogKndError		/**< Error log message */
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
		return QObject::tr("CSA Real-Time");
	}

    //=========================================================================================================
    /**
    * Returns the application name.
    *
    * @return a string containing application name.
    */
    const static QString AppName()
    {
        return QObject::tr("Clinical Sensing and Analysis in Real-Time");
    }

    //=========================================================================================================
    /**
    * Returns the major version number of the application which indicates larger changes of the application.
    *
    * @return the major version number.
    */
    const static int MajorVersion()
    {
        return 0;
    }
    //=========================================================================================================
    /**
    * Returns the minor version number of the application which indicates smaller changes of the application.
    *
    * @return the minor version number.
    */
    const static int MinorVersion()
    {
        return 9;
    }
    //=========================================================================================================
    /**
    * Returns the revision number which indicates the bug fix level.
    *
    * @return the revision number.
    */
    const static int RevisionVersion()
    {
        return 0;
    }
    //=========================================================================================================
    /**
    * Returns the build number which corresponds to the SVN revision control number.
    *
    * @return the build number.
    */
    const static int BuildVersion()
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

#endif // INFOCSART_H
