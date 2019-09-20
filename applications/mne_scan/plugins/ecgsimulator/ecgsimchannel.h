//=============================================================================================================
/**
* @file     ecgsimchannel.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the ECGChannel class.
*
*/

#ifndef ECGSIMCHANNEL_H
#define ECGSIMCHANNEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QVector>
#include <QString>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSIMULATORPLUGIN
//=============================================================================================================

namespace ECGSIMULATORPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS ECGChannel
*
* @brief The ECGChannel class provides a ECG channel.
*/
class ECGSimChannel
{
public:
    typedef QSharedPointer<ECGSimChannel> SPtr;              /**< Shared pointer type for ECGSimChannel. */
    typedef QSharedPointer<const ECGSimChannel> ConstSPtr;   /**< Const shared pointer type for ECGSimChannel. */

    //=========================================================================================================
    /**
    * Constructs a ECGSimChannel.
    *
    * @param [in] ResourceDataPath a string which holds the resource directory where the files are stored which could be used to simulate a channel.
    * @param [in] ChannelFile a string to a specific file which should be used initially.
    * @param [in] enabled whether the channel should be initial enabled.
    * @param [in] visible whether the channel should be initial visible.
    */
    ECGSimChannel(QString ResourceDataPath, QString ChannelFile, bool enabled = true, bool visible = true);

    //=========================================================================================================
    /**
    * Destroys the ECGSimChannel.
    */
    virtual ~ECGSimChannel();


    //=========================================================================================================
    /**
    * Sets the resource directory where the simulation files are stored.
    *
    * @param [in] path a string which holds the path to the folder where the files are stored which could be used to simulate a channel.
    */
    inline void setResourceDataPath(QString& path);

    //=========================================================================================================
    /**
    * Returns the resource directory where the simulation files are stored.
    *
    * @return a string which holds the path to the folder where the files are stored which could be used to simulate a channel.
    */
    inline const QString& getResourceDataPath();

    //=========================================================================================================
    /**
    * Sets the file which should be used to simulate the channel.
    *
    * @param [in] file a string which specifies the file which should be used to simulate the channel.
    */
    inline void setChannelFile(QString file);

    //=========================================================================================================
    /**
    * Returns the file which should be used to simulate the channel.
    *
    * @return a string which specifies the file which should be used to simulate the channel.
    */
    inline const QString& getChannelFile();

    //=========================================================================================================
    /**
    * Returns the samples for simulation.
    *
    * @return a vector which holds all samples of out of the simulation file.
    */
    inline const QVector<double>& getSamples();

    //=========================================================================================================
    /**
    * Sets whether channel is enabled.
    *
    * @param [in] enabled the parameter which enables the channel.
    */
    inline void setEnabled(bool enabled);

    //=========================================================================================================
    /**
    * Returns whether channel is enabled.
    *
    * @return true when the channel is enabled, false otherwise.
    */
    inline bool isEnabled();

    //=========================================================================================================
    /**
    * Sets whether channel is visible.
    *
    * @param [in] visible the parameter which declares the channel visible.
    */
    inline void setVisible(bool visible);

    //=========================================================================================================
    /**
    * Returns whether channel is visible.
    *
    * @return true when the channel is visible, false otherwise.
    */
    inline bool isVisible();

    //=========================================================================================================
    /**
    * Returns the minimum of all simulation sample values.
    *
    * @return the minimal value of all sample values.
    */
    inline double getMinimum();

    //=========================================================================================================
    /**
    * Returns the maximum of all simulation sample values.
    *
    * @return the maximal value of all sample values.
    */
    inline double getMaximum();

    //=========================================================================================================
    /**
    * Initializes the simulation channel.
    */
    void initChannel();

    //=========================================================================================================
    /**
    * Clears the simulation channel.
    */
    void clear();

private:
    QString m_qStringResourceDataPath;  /**< Holds the path to the resource directory.*/

    QString m_qStringChannelFile;   /**< Holds the channel file.*/

    bool m_bIsEnabled;  /**< Holds whether the channel is enabled.*/
    bool m_bIsVisible;  /**< Holds whether the channel is visible.*/

    QVector<double> m_vecBuffer;    /**< A vector which holds all simualtion sample values.*/

    double m_dMin;      /**< Holds minimal sample value.*/
    double m_dMax;      /**< Holds maximal sample value.*/
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

inline void ECGSimChannel::setResourceDataPath(QString& path)
{
    m_qStringResourceDataPath = path;
}


//*************************************************************************************************************

inline const QString& ECGSimChannel::getResourceDataPath()
{
    return m_qStringResourceDataPath;
}


//*************************************************************************************************************

inline void ECGSimChannel::setChannelFile(QString file)
{
    m_qStringChannelFile = file;
}


//*************************************************************************************************************

inline const QString& ECGSimChannel::getChannelFile()
{
    return m_qStringChannelFile;
}


//*************************************************************************************************************

inline const QVector<double>& ECGSimChannel::getSamples()
{
    return m_vecBuffer;
}


//*************************************************************************************************************

inline void ECGSimChannel::setEnabled(bool enabled)
{
    m_bIsEnabled = enabled;
}


//*************************************************************************************************************

inline bool ECGSimChannel::isEnabled()
{
    return m_bIsEnabled;
}


//*************************************************************************************************************

inline void ECGSimChannel::setVisible(bool visible)
{
    m_bIsVisible = visible;
}


//*************************************************************************************************************

inline bool ECGSimChannel::isVisible()
{
    return m_bIsVisible;
}


//*************************************************************************************************************

inline double ECGSimChannel::getMinimum()
{
    return m_dMin;
}


//*************************************************************************************************************

inline double ECGSimChannel::getMaximum()
{
    return m_dMax;
}

}// NAMESPACE

#endif // ECGCHANNEL_H
