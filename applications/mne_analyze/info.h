//=============================================================================================================
/**
* @file     info.cpp
* @author   Franco Polo <Franco-Joel.Polo@tu-ilmenau.de>;
*			Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Franco Polo, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    Contains general application information like: application name and version number.
*
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
        return QObject::tr("MNE_ANALYZE");
    }

    //=========================================================================================================
    /**
    * Returns the application name.
    *
    * @return a string containing application name.
    */
    const static QString AppName()
    {
        return QObject::tr("Co-Registration and Source Localization.");
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
