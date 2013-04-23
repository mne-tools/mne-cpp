//=============================================================================================================
/**
* @file     nomenclature.h
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the whole nomenclature: declaration of PLG_ID class (plugin id's); MSR_ID class (measurement id's) and definitions of new types.
*
*/


#ifndef NOMENCLATURE_H
#define NOMENCLATURE_H


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEF
//=============================================================================================================

typedef unsigned char  U8;	/**< Defines usigned char as U8 type.*/
typedef          char  S8;	/**< Defines char as S8 type.*/
typedef unsigned short U16;	/**< Defines usigned short as U16 type.*/
typedef          short S16;	/**< Defines short as S16 type.*/
typedef unsigned int   U32;	/**< Defines usigned int as U32 type.*/
typedef          int   S32;	/**< Defines int as S32 type.*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../xmeas_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QMap>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XMEASLIB
//=============================================================================================================

namespace XMEASLIB
{

//=============================================================================================================
/**
* DECLARE CLASS PLG_ID
*
* @brief The PLG_ID class provides the plugin id's and check up functions. ToDo Add here new plugin id's!!!
*/
class XMEASSHARED_EXPORT PLG_ID
{
public:
    //=========================================================================================================
    /**
    * Plugin id enumeration. ToDo In next version change this to QMap.
    */
    enum Plugin_ID
    {
        ECGSIM = 0x00010000,                /**< Plugin id of the ECG simulator. */
        ECG = 0x00020000,                   /**< Plugin id of the ECG sensor (Not implemented - just a dummy). */
        RTSERVER = 0x00030000,              /**< Plugin id of the MNE rt server. */
        DUMMYTOOL = 0x00040000,             /**< Plugin id of the dummy toolbox. */
        FILTERTOOL = 0x00050000,            /**< Plugin id of the filter toolbox. */
        GABORPARTICLETOOL = 0x00060000,     /**< Plugin id of the gabor toolbox. */
        SOURCELAB = 0x00070000,             /**< Plugin id of the source lab visualization. */
        BRAINMONITOR = 0x00080000,          /**< Plugin id of the brain monitor visualization. */
        _default = -1                       /**< Default plugin id. */
    };

    //=========================================================================================================
    /**
    * Constructs a Plugin ID with a given ID.
    *
    * @param [in] id value which holds the Plugin ID.
    */
    PLG_ID(int id);

    //=========================================================================================================
    /**
    * Copy constructor which creates a new Plugin ID from a given Plugin ID.
    *
    * @param [in] plg_id reference to the Plugin ID which should be copied.
    */
    PLG_ID(const PLG_ID &plg_id);

    //=========================================================================================================
    /**
    * Constructs a Plugin ID with a given enumeration ID.
    *
    * @param [in] standard_ID value which holds the enumeration Plugin ID.
    */
    PLG_ID(Plugin_ID standard_ID);

    //=========================================================================================================
    /**
    * Destroys the Plugin ID.
    */
    ~PLG_ID(){};

    //=========================================================================================================
    /**
    * Returns a map where the Plugin_ID enumerations are mapped to corresponding strings.
    *
    * @return the map of strings and corresponding Plugin_ID's.
    */
    static QMap<QString, int>  getPluginIDMap()
    {
        QMap<QString, int> map;
        map["ECG Simulator"] = PLG_ID::ECGSIM;
        map["MNE RT Server)"] = PLG_ID::RTSERVER;
        map["Dummy Toolbox"] = PLG_ID::DUMMYTOOL;
        map["Filter Toolbox"] = PLG_ID::FILTERTOOL;
        map["Gabor Particle Toolbox"] = PLG_ID::GABORPARTICLETOOL;
        map["Source Lab"] = PLG_ID::SOURCELAB;
        map["Brain Monitor"] = PLG_ID::BRAINMONITOR;
        return map;
    }

    //=========================================================================================================
    /**
    * Returns the Plugin_ID enumeration of a given index.
    *
    * @param [in] i holds the index.
    *
    * @return the requested Plugin ID.
    */
    Plugin_ID operator[](unsigned int i) const
    { return (Plugin_ID)i; }


    //=========================================================================================================
    /**
    * Assigns the Plugin ID of an other Plugin ID and returns the Plugin ID with the new value.
    *
    * @param [in] other holds the Plugin ID which should be copied.
    *
    * @return the new Plugin ID.
    */
    PLG_ID &operator=(const PLG_ID &other)
    {
        m_S32_ID = other.m_S32_ID;
        return *this;
    }

    //=========================================================================================================
    /**
    * Adds a value to the Plugin ID to increase it and returns the Plugin ID with the new value.
    *
    * @param [in] i the value which should be added to the Plugin ID.
    *
    * @return the new Plugin ID.
    */
    PLG_ID &operator+(int i)
    {
        m_S32_ID = m_S32_ID + i < -1 ? -1 : m_S32_ID + i;
        return *this;
    }

    //=========================================================================================================
    /**
    * Subtracts a value of the Plugin ID to decrease it and returns the Plugin ID with the new value.
    *
    * @param [in] i the value which should be subtracted from the Plugin ID.
    *
    * @return the new Plugin ID.
    */
    PLG_ID &operator-(int i)
    { return (*this + (-1)*i); }

    //=========================================================================================================
    /**
    * Increments (praefix ++x) the Plugin ID and returns the Plugin ID with the new value.
    *
    * @return the new Plugin ID.
    */
    PLG_ID &operator++()//Praefix
    { return (*this+1); }

    //=========================================================================================================
    /**
    * Increments (postfix x++) the Plugin ID and returns the Plugin ID with the new value.
    *
    * @return the new Plugin ID.
    */
    PLG_ID &operator++(int)//Postfix
    { return (*this+1); }

    //=========================================================================================================
    /**
    * Decreases (praefix --x) the Plugin ID and returns the Plugin ID with the new value.
    *
    * @return the new Plugin ID.
    */
    PLG_ID &operator--()//Praefix
    { return (*this-1); }

    //=========================================================================================================
    /**
    * Decreases (postfix x--) the Plugin ID and returns the Plugin ID with the new value.
    *
    * @return the new Plugin ID.
    */
    PLG_ID &operator--(int)//Postfix
    { return (*this-1); }

    //=========================================================================================================
    /**
    * Compares the Plugin ID with another Plugin ID returns the result.
    *
    * @param [in] other holds the other Plugin ID.
    *
    * @return true when the Plugin ID's are equal, false otherwise.
    */
    bool operator==(const PLG_ID &other) const
    {return m_S32_ID==other.m_S32_ID;}

    //=========================================================================================================
    /**
    * Compares the Plugin ID with another Plugin ID whether they are not eqal returns the result.
    *
    * @param [in] other holds the other Plugin ID.
    *
    * @return true when the Plugin ID's aren't equal, false otherwise.
    */
    inline bool operator!= (const PLG_ID &other) const
    { return !(*this == other); }

    //=========================================================================================================
    /**
    * Compares the Plugin ID with another Plugin ID whether it's smaller.
    *
    * @param [in] other holds the other Plugin ID.
    *
    * @return true when the Plugin ID is smaller, false otherwise.
    */
    bool operator< (const PLG_ID &other) const
    { return m_S32_ID<other.m_S32_ID;}

    //=========================================================================================================
    /**
    * Compares the Plugin ID with another Plugin ID whether it's bigger.
    *
    * @param [in] other holds the other Plugin ID.
    *
    * @return true when the Plugin ID is bigger, false otherwise.
    */
    inline bool operator> (const PLG_ID &other) const
    { return other < *this; }

    //=========================================================================================================
    /**
    * Compares the Plugin ID with another Plugin ID whether it's smaller or equal.
    *
    * @param [in] other holds the other Plugin ID.
    *
    * @return true when the Plugin ID is smaller or equal, false otherwise.
    */
    inline bool operator<= (const PLG_ID &other) const
    { return m_S32_ID<=other.m_S32_ID; }

    //=========================================================================================================
    /**
    * Compares the Plugin ID with another Plugin ID whether it's bigger or equal.
    *
    * @param [in] other holds the other Plugin ID.
    *
    * @return true when the Plugin ID is bigger or equal, false otherwise.
    */
    inline bool operator>= (const PLG_ID &other) const
    { return m_S32_ID>=other.m_S32_ID; }

    //=========================================================================================================
    /**
    * Returns the Plugin_ID in integer format.
    *
    * @return the Plugin ID.
    */
    inline int getIntID()
    { return m_S32_ID; }

    //=========================================================================================================
    /**
    * Returns the Plugin_ID as an enumeration.
    *
    * @return the Plugin ID as an enumeration.
    */
    inline Plugin_ID getPluginID()
    { return (Plugin_ID)m_S32_ID; }

private:
    S32 m_S32_ID;	/**< Holds the Plugin ID.*/


};

//=========================================================================================================
/**
* DECLARE CLASS MSR_ID
*
* @brief The MSR_ID class provides the measurement id's and check up functions. ToDo Add here new measurement ID's!!!
*/
class XMEASSHARED_EXPORT MSR_ID
{
public:
    //=========================================================================================================
    /**
    * Measurement id enumeration. ToDo In next version change this to QMap.
    */
    enum Measurement_ID
    {
        // ECG
        ECGSIM_I = PLG_ID::ECGSIM,      /**< Measurement id of the ECG I channel. */
        ECGSIM_II,                      /**< Measurement id of the ECG II channel. */
        ECGSIM_III,                     /**< Measurement id of the ECG III channel. */

        // RTSERVER
        MEGRTSERVER_OUTPUT = PLG_ID::RTSERVER,   /**< Measurement id of a MEG channel. */

        // DummyToolbox
        DUMMYTOOL_OUTPUT = PLG_ID::DUMMYTOOL,   /**< Measurement id of the dummy tool box output channel. */
        DUMMYTOOL_OUTPUT_II,                    /**< Measurement id of the dummy tool box output channel II. */

        // FilterToolbox
        FILTERTOOL_INPUT = PLG_ID::FILTERTOOL,  /**< Measurement id of the filter tool box input channel. */
        FILTERTOOL_OUTPUT,                      /**< Measurement id of the filter tool box output channel. */

        // GaborParticleToolbox
        GABORPARTICLETOOL_CURRENT = PLG_ID::GABORPARTICLETOOL,  /**< Measurement id of the gabor particle tool box output channel. */
        GABORPARTICLETOOL_FREQUENCY,                            /**< Measurement id of the gabor particle tool box estimated frequency. */
        GABORPARTICLETOOL_FREQUENCY_STD,                        /**< Measurement id of the gabor particle tool box estimated standard deviation of the frequency. */
        GABORPARTICLETOOL_SCALE,                                /**< Measurement id of the gabor particle tool box estimated scale of the particles. */
        GABORPARTICLETOOL_SCALE_STD,                            /**< Measurement id of the gabor particle tool box estimated standard deviation of the scale of the particles. */

        // SourceLab
        SOURCELAB_OUTPUT = PLG_ID::SOURCELAB,   /**< Measurement id of the source lab output channel. */

        // BarinMonitor
        BRAINMONITOR_OUTPUT = PLG_ID::BRAINMONITOR,         /**< Measurement id of the brain monitor output channel. */

        //Default
        _default = -1       /**< Default measurement id. */
    };

    //=========================================================================================================
    /**
    * Constructs a Measurement ID with a given id.
    *
    * @param [in] id value which holds the Measurement ID.
    */
    MSR_ID(int id);

    //=========================================================================================================
    /**
    * Copy constructor which creates a new Measurement ID from a given Measurement ID.
    *
    * @param [in] msr_id reference to the Measurement ID which should be copied.
    */
    MSR_ID(const MSR_ID &msr_id);

    //=========================================================================================================
    /**
    * Constructs a Measurement ID with a given enumeration ID.
    *
    * @param [in] standard_ID value which holds the enumeration Measurement ID.
    */
    MSR_ID(Measurement_ID standard_ID);

    //=========================================================================================================
    /**
    * Destroys the Measurement ID.
    */
    ~MSR_ID(){};

    //=========================================================================================================
    /**
    * Returns a List which holds corresponding to all plugins a string list with all measurements.
    *
    * @return the list of string list's.
    */
    static QList<QStringList> getMeasurementIDStrings()
    {
        QList<QStringList> list;
        QStringList list1, list2, list3, list4, list5;

        list1 << "ECGSIM_I" << "ECGSIM_II" << "ECGSIM_III";
        list2 << "MEGSIM_CHAN";

        for (int i = 0; i < 320; ++i)
            list3 << QString("MEG_%1").arg(i);

        for (int i = 0; i < 20; ++i)
            list3 << QString("STI_%1").arg(i);

        for (int i = 0; i < 252; ++i)
            list3 << QString("EEG_%1").arg(i);

        for (int i = 0; i < 20; ++i)
            list3 << QString("EOG_%1").arg(i);

        list4 << "FILTERTOOL_INPUT" << "FILTERTOOL_OUTPUT";
        list5 << "GABORPARTICLETOOL_CURRENT" << "GABORPARTICLETOOL_FREQUENCY";

        list << list1 << list2 << list3 << list4 << list5;

        return list;
    }

    //=========================================================================================================
    /**
    * Returns the Measurement_ID enumeration of a given index.
    *
    * @param [in] i holds the index.
    *
    * @return the requested Measurement ID.
    */
    Measurement_ID operator[](unsigned int i) const
    { return (Measurement_ID)i; }

    //=========================================================================================================
    /**
    * Assigns the Measurement ID  of an other Measurement ID and returns the Measurement ID with the new value.
    *
    * @param [in] other holds the Measurement ID which should be copied.
    *
    * @return the new Measurement ID.
    */
    MSR_ID &operator=(const MSR_ID &other)
    {
        m_S32_ID = other.m_S32_ID;
        return *this;
    }

    //=========================================================================================================
    /**
    * Adds a value to the Measurement ID to increase it and returns the Measurement ID with the new value.
    *
    * @param [in] i the value which should be added to the Measurement ID.
    *
    * @return the new Measurement ID.
    */
    MSR_ID &operator+(int i)
    {
        m_S32_ID = m_S32_ID + i < -1 ? -1 : m_S32_ID + i;
        return *this;
    }

    //=========================================================================================================
    /**
    * Subtracts a value of the Measurement ID to decrease it and returns the Measurement ID with the new value.
    *
    * @param [in] i the value which should be subtracted from the Measurement ID.
    *
    * @return the new Measurement ID.
    */
    MSR_ID &operator-(int i)
    { return (*this + (-1)*i); }

    //=========================================================================================================
    /**
    * Increments (praefix ++x) the Measurement ID and returns the Measurement ID with the new value.
    *
    * @return the new Measurement ID.
    */
    MSR_ID &operator++()//Praefix
    { return (*this+1); }

    //=========================================================================================================
    /**
    * Increments (postfix x++) the Measurement ID and returns the Measurement ID with the new value.
    *
    * @return the new Measurement ID.
    */
    MSR_ID &operator++(int)//Postfix
    { return (*this+1); }

    //=========================================================================================================
    /**
    * Decreases (praefix --x) the Measurement ID and returns the Measurement ID with the new value.
    *
    * @return the new Measurement ID.
    */
    MSR_ID &operator--()//Praefix
    { return (*this-1); }

    //=========================================================================================================
    /**
    * Decreases (postfix x--) the Measurement ID and returns the Measurement ID with the new value.
    *
    * @return the new Measurement ID.
    */
    MSR_ID &operator--(int)//Postfix
    { return (*this-1); }

    //=========================================================================================================
    /**
    * Compares the Measurement ID with another Measurement ID returns the result.
    *
    * @param [in] other holds the other Measurement ID.
    *
    * @return true when the Measurement ID's are equal, false otherwise.
    */
    bool operator==(const MSR_ID &other) const
    {return m_S32_ID==other.m_S32_ID;}

    //=========================================================================================================
    /**
    * Compares the Measurement ID with another Measurement ID whether they are not eqal returns the result.
    *
    * @param [in] other holds the other Measurement ID.
    *
    * @return true when the Measurement ID's aren't equal, false otherwise.
    */
    inline bool operator!= (const MSR_ID &other) const
    { return !(*this == other); }

    //=========================================================================================================
    /**
    * Compares the Measurement ID with another Measurement IDwhether it's smaller.
    *
    * @param [in] other holds the other Measurement ID.
    *
    * @return true when the Measurement ID is smaller, false otherwise.
    */
    bool operator< (const MSR_ID &other) const
    { return m_S32_ID<other.m_S32_ID;}

    //=========================================================================================================
    /**
    * Compares the Measurement ID with another Measurement ID whether it's bigger.
    *
    * @param [in] other holds the other Measurement ID.
    *
    * @return true when the Measurement ID is bigger, false otherwise.
    */
    inline bool operator> (const MSR_ID &other) const
    { return other < *this; }

    //=========================================================================================================
    /**
    * Compares the Measurement ID with another Measurement ID whether it's smaller or equal.
    *
    * @param [in] other holds the other Measurement ID.
    *
    * @return true when the Measurement ID is smaller or equal, false otherwise.
    */
    inline bool operator<= (const MSR_ID &other) const
    { return m_S32_ID<=other.m_S32_ID; }

    //=========================================================================================================
    /**
    * Compares the Measurement ID with another Measurement ID whether it's bigger or equal.
    *
    * @param [in] other holds the other Measurement ID.
    *
    * @return true when the Measurement ID is bigger or equal, false otherwise.
    */
    inline bool operator>= (const MSR_ID &other) const
    { return m_S32_ID>=other.m_S32_ID; }

    //=========================================================================================================
    /**
    * Returns the Measurement_ID in integer format.
    *
    * @return the Measurement ID.
    */
    inline int getIntID()
    { return m_S32_ID; }

    //=========================================================================================================
    /**
    * Returns the Measurement_ID as an enumeration.
    *
    * @return the Measurement ID as an enumeration.
    */
    inline Measurement_ID getMeasurementID()
    { return (Measurement_ID)m_S32_ID; }

private:
    S32 m_S32_ID;	/**< Holds the Measurement ID.*/

};

}//Namespace

#endif // NOMENCLATURE_H
