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
* @brief    Contains the whole nomenclature: declaration of MDL_ID class (module id's); MSR_ID class (measurement id's) and definitions of new types.
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
* DECLARE CLASS MDL_ID
*
* @brief The MDL_ID class provides the module id's and check up functions. ToDo Add here new module id's!!!
*/
class XMEASSHARED_EXPORT MDL_ID
{
public:
    //=========================================================================================================
    /**
    * Module id enumeration. ToDo In next version change this to QMap.
    */
    enum Module_ID
    {
        ECGSIM = 0x00010000,                /**< Plugin id of the ECG simulator. */
        ECG = 0x00020000,                   /**< Plugin id of the ECG sensor (Not implemented - just a dummy). */
        RTSERVER = 0x00030000,              /**< Plugin id of the MNE rt server. */
        DUMMYTOOL = 0x00040000,             /**< Plugin id of the dummy toolbox. */
        FILTERTOOL = 0x00050000,            /**< Plugin id of the filter toolbox. */
        GABORPARTICLETOOL = 0x00060000,     /**< Plugin id of the gabor toolbox. */
        BRAINMONITOR = 0x00070000,          /**< Plugin id of the brain monitor visualization. */
        _default = -1                       /**< Default module id. */
    };

    //=========================================================================================================
    /**
    * Constructs a Module ID with a given ID.
    *
    * @param [in] id value which holds the Module ID.
    */
    MDL_ID(int id);

    //=========================================================================================================
    /**
    * Copy constructor which creates a new Module ID from a given Module ID.
    *
    * @param [in] mdl_id reference to the Module ID which should be copied.
    */
    MDL_ID(const MDL_ID &mdl_id);

    //=========================================================================================================
    /**
    * Constructs a Module ID with a given enumeration ID.
    *
    * @param [in] standard_ID value which holds the enumeration Module ID.
    */
    MDL_ID(Module_ID standard_ID);

    //=========================================================================================================
    /**
    * Destroys the Module ID.
    */
    ~MDL_ID(){};

    //=========================================================================================================
    /**
    * Returns a map where the Module_ID enumerations are mapped to corresponding strings.
    *
    * @return the map of strings and corresponding Module_ID's.
    */
    static QMap<QString, int>  getModuleIDMap()
    {
        QMap<QString, int> map;
        map["ECG Simulator"] = MDL_ID::ECGSIM;
        map["MNE RT Server)"] = MDL_ID::RTSERVER;
        map["Dummy Toolbox"] = MDL_ID::DUMMYTOOL;
        map["Filter Toolbox"] = MDL_ID::FILTERTOOL;
        map["Gabor Particle Toolbox"] = MDL_ID::GABORPARTICLETOOL;
        map["Brain Monitor"] = MDL_ID::BRAINMONITOR;
        return map;
    }

    //=========================================================================================================
    /**
    * Returns the Module_ID enumeration of a given index.
    *
    * @param [in] i holds the index.
    *
    * @return the requested Module ID.
    */
    Module_ID operator[](unsigned int i) const
    { return (Module_ID)i; }


    //=========================================================================================================
    /**
    * Assigns the Module ID of an other Module ID and returns the Module ID with the new value.
    *
    * @param [in] other holds the Module ID which should be copied.
    *
    * @return the new Module ID.
    */
    MDL_ID &operator=(const MDL_ID &other)
    {
        m_S32_ID = other.m_S32_ID;
        return *this;
    }

    //=========================================================================================================
    /**
    * Adds a value to the Module ID to increase it and returns the Module ID with the new value.
    *
    * @param [in] i the value which should be added to the Module ID.
    *
    * @return the new Module ID.
    */
    MDL_ID &operator+(int i)
    {
        m_S32_ID = m_S32_ID + i < -1 ? -1 : m_S32_ID + i;
        return *this;
    }

    //=========================================================================================================
    /**
    * Subtracts a value of the Module ID to decrease it and returns the Module ID with the new value.
    *
    * @param [in] i the value which should be subtracted from the Module ID.
    *
    * @return the new Module ID.
    */
    MDL_ID &operator-(int i)
    { return (*this + (-1)*i); }

    //=========================================================================================================
    /**
    * Increments (praefix ++x) the Module ID and returns the Module ID with the new value.
    *
    * @return the new Module ID.
    */
    MDL_ID &operator++()//Praefix
    { return (*this+1); }

    //=========================================================================================================
    /**
    * Increments (postfix x++) the Module ID and returns the Module ID with the new value.
    *
    * @return the new Module ID.
    */
    MDL_ID &operator++(int)//Postfix
    { return (*this+1); }

    //=========================================================================================================
    /**
    * Decreases (praefix --x) the Module ID and returns the Module ID with the new value.
    *
    * @return the new Module ID.
    */
    MDL_ID &operator--()//Praefix
    { return (*this-1); }

    //=========================================================================================================
    /**
    * Decreases (postfix x--) the Module ID and returns the Module ID with the new value.
    *
    * @return the new Module ID.
    */
    MDL_ID &operator--(int)//Postfix
    { return (*this-1); }

    //=========================================================================================================
    /**
    * Compares the Module ID with another Module ID returns the result.
    *
    * @param [in] other holds the other Module ID.
    *
    * @return true when the Module ID's are equal, false otherwise.
    */
    bool operator==(const MDL_ID &other) const
    {return m_S32_ID==other.m_S32_ID;}

    //=========================================================================================================
    /**
    * Compares the Module ID with another Module ID whether they are not eqal returns the result.
    *
    * @param [in] other holds the other Module ID.
    *
    * @return true when the Module ID's aren't equal, false otherwise.
    */
    inline bool operator!= (const MDL_ID &other) const
    { return !(*this == other); }

    //=========================================================================================================
    /**
    * Compares the Module ID with another Module ID whether it's smaller.
    *
    * @param [in] other holds the other Module ID.
    *
    * @return true when the Module ID is smaller, false otherwise.
    */
    bool operator< (const MDL_ID &other) const
    { return m_S32_ID<other.m_S32_ID;}

    //=========================================================================================================
    /**
    * Compares the Module ID with another Module ID whether it's bigger.
    *
    * @param [in] other holds the other Module ID.
    *
    * @return true when the Module ID is bigger, false otherwise.
    */
    inline bool operator> (const MDL_ID &other) const
    { return other < *this; }

    //=========================================================================================================
    /**
    * Compares the Module ID with another Module ID whether it's smaller or equal.
    *
    * @param [in] other holds the other Module ID.
    *
    * @return true when the Module ID is smaller or equal, false otherwise.
    */
    inline bool operator<= (const MDL_ID &other) const
    { return m_S32_ID<=other.m_S32_ID; }

    //=========================================================================================================
    /**
    * Compares the Module ID with another Module ID whether it's bigger or equal.
    *
    * @param [in] other holds the other Module ID.
    *
    * @return true when the Module ID is bigger or equal, false otherwise.
    */
    inline bool operator>= (const MDL_ID &other) const
    { return m_S32_ID>=other.m_S32_ID; }

    //=========================================================================================================
    /**
    * Returns the Module_ID in integer format.
    *
    * @return the Module ID.
    */
    inline int getIntID()
    { return m_S32_ID; }

    //=========================================================================================================
    /**
    * Returns the Module_ID as an enumeration.
    *
    * @return the Module ID as an enumeration.
    */
    inline Module_ID getModuleID()
    { return (Module_ID)m_S32_ID; }

private:
    S32 m_S32_ID;	/**< Holds the Module ID.*/


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
        ECGSIM_I = MDL_ID::ECGSIM,      /**< Measurement id of the ECG I channel. */
        ECGSIM_II,                      /**< Measurement id of the ECG II channel. */
        ECGSIM_III,                     /**< Measurement id of the ECG III channel. */

        // RTSERVER
        MEGRTSERVER_OUTPUT = MDL_ID::RTSERVER,   /**< Measurement id of a MEG channel. */

        // DummyToolbox
        DUMMYTOOL_OUTPUT = MDL_ID::DUMMYTOOL,   /**< Measurement id of the dummy tool box output channel. */
        DUMMYTOOL_OUTPUT_II,                    /**< Measurement id of the dummy tool box output channel II. */

        // FilterToolbox
        FILTERTOOL_INPUT = MDL_ID::FILTERTOOL,  /**< Measurement id of the filter tool box input channel. */
        FILTERTOOL_OUTPUT,                      /**< Measurement id of the filter tool box output channel. */

        // GaborParticleToolbox
        GABORPARTICLETOOL_CURRENT = MDL_ID::GABORPARTICLETOOL,  /**< Measurement id of the gabor particle tool box output channel. */
        GABORPARTICLETOOL_FREQUENCY,                            /**< Measurement id of the gabor particle tool box estimated frequency. */
        GABORPARTICLETOOL_FREQUENCY_STD,                        /**< Measurement id of the gabor particle tool box estimated standard deviation of the frequency. */
        GABORPARTICLETOOL_SCALE,                                /**< Measurement id of the gabor particle tool box estimated scale of the particles. */
        GABORPARTICLETOOL_SCALE_STD,                            /**< Measurement id of the gabor particle tool box estimated standard deviation of the scale of the particles. */

        // BarinMonitor
        BRAINMONITOR_OUTPUT = MDL_ID::BRAINMONITOR,         /**< Measurement id of the brain monitor output channel. */


        //Default
        _default = -1		/**< Default measurement id. */
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
    * Returns a List which holds corresponding to all modules a string list with all measurements.
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
