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

#include "../rtmeas_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QMap>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTMEASLIB
//=============================================================================================================

namespace RTMEASLIB
{

//=============================================================================================================
/**
* DECLARE CLASS MDL_ID
*
* @brief The MDL_ID class provides the module id's and check up functions. ToDo Add here new module id's!!!
*/
class RTMEASSHARED_EXPORT MDL_ID
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
        MEGSIM = 0x00030000,                /**< Plugin id of the MEG channel simulator. */
        MEG = 0x00040000,                   /**< Plugin id of the MEG (rtproc) - Neuromag. */
        DUMMYTOOL = 0x00050000,             /**< Plugin id of the dummy toolbox. */
        FILTERTOOL = 0x00060000,            /**< Plugin id of the filter toolbox. */
        GABORPARTICLETOOL = 0x00070000,     /**< Plugin id of the gabor toolbox. */
        BRAINMONITOR = 0x00080000,          /**< Plugin id of the brain monitor visualization. */
        RTSERVER = 0x00080000,              /**< Plugin id of the rt server. */
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
        map["MEG Simulator"] = MDL_ID::MEGSIM;
        map["MEG (rtproc)"] = MDL_ID::MEG;
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
class RTMEASSHARED_EXPORT MSR_ID
{
public:
    //=========================================================================================================
    /**
    * Measurement id enumeration. ToDo In next version change this to QMap.
    */
    enum Measurement_ID
    {
        // ECG
        ECGSIM_I = MDL_ID::ECGSIM,		/**< Measurement id of the ECG I channel. */
        ECGSIM_II,						/**< Measurement id of the ECG II channel. */
        ECGSIM_III,						/**< Measurement id of the ECG III channel. */

        // MEGSIM
        MEGSIM_CHAN = MDL_ID::MEGSIM,	/**< Measurement id of the MEG simulator channel. */
        MEGSIM,                    /**< Measurement id of the MEG simulator channels. */

        // MEG
        MEG_0 = MDL_ID::MEG,		/**< Measurement id of a MEG channel. */
        MEG_1,						/**< Measurement id of a MEG channel. */
        MEG_2,						/**< Measurement id of a MEG channel. */
        MEG_3,						/**< Measurement id of a MEG channel. */
        MEG_4,						/**< Measurement id of a MEG channel. */
        MEG_5,						/**< Measurement id of a MEG channel. */
        MEG_6,						/**< Measurement id of a MEG channel. */
        MEG_7,						/**< Measurement id of a MEG channel. */
        MEG_8,						/**< Measurement id of a MEG channel. */
        MEG_9,						/**< Measurement id of a MEG channel. */
        MEG_10,						/**< Measurement id of a MEG channel. */
        MEG_11,						/**< Measurement id of a MEG channel. */
        MEG_12,						/**< Measurement id of a MEG channel. */
        MEG_13,						/**< Measurement id of a MEG channel. */
        MEG_14,						/**< Measurement id of a MEG channel. */
        MEG_15,						/**< Measurement id of a MEG channel. */
        MEG_16,						/**< Measurement id of a MEG channel. */
        MEG_17,						/**< Measurement id of a MEG channel. */
        MEG_18,						/**< Measurement id of a MEG channel. */
        MEG_19,						/**< Measurement id of a MEG channel. */
        MEG_20,						/**< Measurement id of a MEG channel. */
        MEG_21,						/**< Measurement id of a MEG channel. */
        MEG_22,						/**< Measurement id of a MEG channel. */
        MEG_23,						/**< Measurement id of a MEG channel. */
        MEG_24,						/**< Measurement id of a MEG channel. */
        MEG_25,						/**< Measurement id of a MEG channel. */
        MEG_26,						/**< Measurement id of a MEG channel. */
        MEG_27,						/**< Measurement id of a MEG channel. */
        MEG_28,						/**< Measurement id of a MEG channel. */
        MEG_29,						/**< Measurement id of a MEG channel. */
        MEG_30,						/**< Measurement id of a MEG channel. */
        MEG_31,						/**< Measurement id of a MEG channel. */
        MEG_32,						/**< Measurement id of a MEG channel. */
        MEG_33,						/**< Measurement id of a MEG channel. */
        MEG_34,						/**< Measurement id of a MEG channel. */
        MEG_35,						/**< Measurement id of a MEG channel. */
        MEG_36,						/**< Measurement id of a MEG channel. */
        MEG_37,						/**< Measurement id of a MEG channel. */
        MEG_38,						/**< Measurement id of a MEG channel. */
        MEG_39,						/**< Measurement id of a MEG channel. */
        MEG_40,						/**< Measurement id of a MEG channel. */
        MEG_41,						/**< Measurement id of a MEG channel. */
        MEG_42,						/**< Measurement id of a MEG channel. */
        MEG_43,						/**< Measurement id of a MEG channel. */
        MEG_44,						/**< Measurement id of a MEG channel. */
        MEG_45,						/**< Measurement id of a MEG channel. */
        MEG_46,						/**< Measurement id of a MEG channel. */
        MEG_47,						/**< Measurement id of a MEG channel. */
        MEG_48,						/**< Measurement id of a MEG channel. */
        MEG_49,						/**< Measurement id of a MEG channel. */
        MEG_50,						/**< Measurement id of a MEG channel. */
        MEG_51,						/**< Measurement id of a MEG channel. */
        MEG_52,						/**< Measurement id of a MEG channel. */
        MEG_53,						/**< Measurement id of a MEG channel. */
        MEG_54,						/**< Measurement id of a MEG channel. */
        MEG_55,						/**< Measurement id of a MEG channel. */
        MEG_56,						/**< Measurement id of a MEG channel. */
        MEG_57,						/**< Measurement id of a MEG channel. */
        MEG_58,						/**< Measurement id of a MEG channel. */
        MEG_59,						/**< Measurement id of a MEG channel. */
        MEG_60,						/**< Measurement id of a MEG channel. */
        MEG_61,						/**< Measurement id of a MEG channel. */
        MEG_62,						/**< Measurement id of a MEG channel. */
        MEG_63,						/**< Measurement id of a MEG channel. */
        MEG_64,						/**< Measurement id of a MEG channel. */
        MEG_65,						/**< Measurement id of a MEG channel. */
        MEG_66,						/**< Measurement id of a MEG channel. */
        MEG_67,						/**< Measurement id of a MEG channel. */
        MEG_68,						/**< Measurement id of a MEG channel. */
        MEG_69,						/**< Measurement id of a MEG channel. */
        MEG_70,						/**< Measurement id of a MEG channel. */
        MEG_71,						/**< Measurement id of a MEG channel. */
        MEG_72,						/**< Measurement id of a MEG channel. */
        MEG_73,						/**< Measurement id of a MEG channel. */
        MEG_74,						/**< Measurement id of a MEG channel. */
        MEG_75,						/**< Measurement id of a MEG channel. */
        MEG_76,						/**< Measurement id of a MEG channel. */
        MEG_77,						/**< Measurement id of a MEG channel. */
        MEG_78,						/**< Measurement id of a MEG channel. */
        MEG_79,						/**< Measurement id of a MEG channel. */
        MEG_80,						/**< Measurement id of a MEG channel. */
        MEG_81,						/**< Measurement id of a MEG channel. */
        MEG_82,						/**< Measurement id of a MEG channel. */
        MEG_83,						/**< Measurement id of a MEG channel. */
        MEG_84,						/**< Measurement id of a MEG channel. */
        MEG_85,						/**< Measurement id of a MEG channel. */
        MEG_86,						/**< Measurement id of a MEG channel. */
        MEG_87,						/**< Measurement id of a MEG channel. */
        MEG_88,						/**< Measurement id of a MEG channel. */
        MEG_89,						/**< Measurement id of a MEG channel. */
        MEG_90,						/**< Measurement id of a MEG channel. */
        MEG_91,						/**< Measurement id of a MEG channel. */
        MEG_92,						/**< Measurement id of a MEG channel. */
        MEG_93,						/**< Measurement id of a MEG channel. */
        MEG_94,						/**< Measurement id of a MEG channel. */
        MEG_95,						/**< Measurement id of a MEG channel. */
        MEG_96,						/**< Measurement id of a MEG channel. */
        MEG_97,						/**< Measurement id of a MEG channel. */
        MEG_98,						/**< Measurement id of a MEG channel. */
        MEG_99,						/**< Measurement id of a MEG channel. */
        MEG_100,					/**< Measurement id of a MEG channel. */
        MEG_101,					/**< Measurement id of a MEG channel. */
        MEG_102,					/**< Measurement id of a MEG channel. */
        MEG_103,					/**< Measurement id of a MEG channel. */
        MEG_104,					/**< Measurement id of a MEG channel. */
        MEG_105,					/**< Measurement id of a MEG channel. */
        MEG_106,					/**< Measurement id of a MEG channel. */
        MEG_107,					/**< Measurement id of a MEG channel. */
        MEG_108,					/**< Measurement id of a MEG channel. */
        MEG_109,					/**< Measurement id of a MEG channel. */
        MEG_110,					/**< Measurement id of a MEG channel. */
        MEG_111,					/**< Measurement id of a MEG channel. */
        MEG_112,					/**< Measurement id of a MEG channel. */
        MEG_113,					/**< Measurement id of a MEG channel. */
        MEG_114,					/**< Measurement id of a MEG channel. */
        MEG_115,					/**< Measurement id of a MEG channel. */
        MEG_116,					/**< Measurement id of a MEG channel. */
        MEG_117,					/**< Measurement id of a MEG channel. */
        MEG_118,					/**< Measurement id of a MEG channel. */
        MEG_119,					/**< Measurement id of a MEG channel. */
        MEG_120,					/**< Measurement id of a MEG channel. */
        MEG_121,					/**< Measurement id of a MEG channel. */
        MEG_122,					/**< Measurement id of a MEG channel. */
        MEG_123,					/**< Measurement id of a MEG channel. */
        MEG_124,					/**< Measurement id of a MEG channel. */
        MEG_125,					/**< Measurement id of a MEG channel. */
        MEG_126,					/**< Measurement id of a MEG channel. */
        MEG_127,					/**< Measurement id of a MEG channel. */
        MEG_128,					/**< Measurement id of a MEG channel. */
        MEG_129,					/**< Measurement id of a MEG channel. */
        MEG_130,					/**< Measurement id of a MEG channel. */
        MEG_131,					/**< Measurement id of a MEG channel. */
        MEG_132,					/**< Measurement id of a MEG channel. */
        MEG_133,					/**< Measurement id of a MEG channel. */
        MEG_134,					/**< Measurement id of a MEG channel. */
        MEG_135,					/**< Measurement id of a MEG channel. */
        MEG_136,					/**< Measurement id of a MEG channel. */
        MEG_137,					/**< Measurement id of a MEG channel. */
        MEG_138,					/**< Measurement id of a MEG channel. */
        MEG_139,					/**< Measurement id of a MEG channel. */
        MEG_140,					/**< Measurement id of a MEG channel. */
        MEG_141,					/**< Measurement id of a MEG channel. */
        MEG_142,					/**< Measurement id of a MEG channel. */
        MEG_143,					/**< Measurement id of a MEG channel. */
        MEG_144,					/**< Measurement id of a MEG channel. */
        MEG_145,					/**< Measurement id of a MEG channel. */
        MEG_146,					/**< Measurement id of a MEG channel. */
        MEG_147,					/**< Measurement id of a MEG channel. */
        MEG_148,					/**< Measurement id of a MEG channel. */
        MEG_149,					/**< Measurement id of a MEG channel. */
        MEG_150,					/**< Measurement id of a MEG channel. */
        MEG_151,					/**< Measurement id of a MEG channel. */
        MEG_152,					/**< Measurement id of a MEG channel. */
        MEG_153,					/**< Measurement id of a MEG channel. */
        MEG_154,					/**< Measurement id of a MEG channel. */
        MEG_155,					/**< Measurement id of a MEG channel. */
        MEG_156,					/**< Measurement id of a MEG channel. */
        MEG_157,					/**< Measurement id of a MEG channel. */
        MEG_158,					/**< Measurement id of a MEG channel. */
        MEG_159,					/**< Measurement id of a MEG channel. */
        MEG_160,					/**< Measurement id of a MEG channel. */
        MEG_161,					/**< Measurement id of a MEG channel. */
        MEG_162,					/**< Measurement id of a MEG channel. */
        MEG_163,					/**< Measurement id of a MEG channel. */
        MEG_164,					/**< Measurement id of a MEG channel. */
        MEG_165,					/**< Measurement id of a MEG channel. */
        MEG_166,					/**< Measurement id of a MEG channel. */
        MEG_167,					/**< Measurement id of a MEG channel. */
        MEG_168,					/**< Measurement id of a MEG channel. */
        MEG_169,					/**< Measurement id of a MEG channel. */
        MEG_170,					/**< Measurement id of a MEG channel. */
        MEG_171,					/**< Measurement id of a MEG channel. */
        MEG_172,					/**< Measurement id of a MEG channel. */
        MEG_173,					/**< Measurement id of a MEG channel. */
        MEG_174,					/**< Measurement id of a MEG channel. */
        MEG_175,					/**< Measurement id of a MEG channel. */
        MEG_176,					/**< Measurement id of a MEG channel. */
        MEG_177,					/**< Measurement id of a MEG channel. */
        MEG_178,					/**< Measurement id of a MEG channel. */
        MEG_179,					/**< Measurement id of a MEG channel. */
        MEG_180,					/**< Measurement id of a MEG channel. */
        MEG_181,					/**< Measurement id of a MEG channel. */
        MEG_182,					/**< Measurement id of a MEG channel. */
        MEG_183,					/**< Measurement id of a MEG channel. */
        MEG_184,					/**< Measurement id of a MEG channel. */
        MEG_185,					/**< Measurement id of a MEG channel. */
        MEG_186,					/**< Measurement id of a MEG channel. */
        MEG_187,					/**< Measurement id of a MEG channel. */
        MEG_188,					/**< Measurement id of a MEG channel. */
        MEG_189,					/**< Measurement id of a MEG channel. */
        MEG_190,					/**< Measurement id of a MEG channel. */
        MEG_191,					/**< Measurement id of a MEG channel. */
        MEG_192,					/**< Measurement id of a MEG channel. */
        MEG_193,					/**< Measurement id of a MEG channel. */
        MEG_194,					/**< Measurement id of a MEG channel. */
        MEG_195,					/**< Measurement id of a MEG channel. */
        MEG_196,					/**< Measurement id of a MEG channel. */
        MEG_197,					/**< Measurement id of a MEG channel. */
        MEG_198,					/**< Measurement id of a MEG channel. */
        MEG_199,					/**< Measurement id of a MEG channel. */
        MEG_200,					/**< Measurement id of a MEG channel. */
        MEG_201,					/**< Measurement id of a MEG channel. */
        MEG_202,					/**< Measurement id of a MEG channel. */
        MEG_203,					/**< Measurement id of a MEG channel. */
        MEG_204,					/**< Measurement id of a MEG channel. */
        MEG_205,					/**< Measurement id of a MEG channel. */
        MEG_206,					/**< Measurement id of a MEG channel. */
        MEG_207,					/**< Measurement id of a MEG channel. */
        MEG_208,					/**< Measurement id of a MEG channel. */
        MEG_209,					/**< Measurement id of a MEG channel. */
        MEG_210,					/**< Measurement id of a MEG channel. */
        MEG_211,					/**< Measurement id of a MEG channel. */
        MEG_212,					/**< Measurement id of a MEG channel. */
        MEG_213,					/**< Measurement id of a MEG channel. */
        MEG_214,					/**< Measurement id of a MEG channel. */
        MEG_215,					/**< Measurement id of a MEG channel. */
        MEG_216,					/**< Measurement id of a MEG channel. */
        MEG_217,					/**< Measurement id of a MEG channel. */
        MEG_218,					/**< Measurement id of a MEG channel. */
        MEG_219,					/**< Measurement id of a MEG channel. */
        MEG_220,					/**< Measurement id of a MEG channel. */
        MEG_221,					/**< Measurement id of a MEG channel. */
        MEG_222,					/**< Measurement id of a MEG channel. */
        MEG_223,					/**< Measurement id of a MEG channel. */
        MEG_224,					/**< Measurement id of a MEG channel. */
        MEG_225,					/**< Measurement id of a MEG channel. */
        MEG_226,					/**< Measurement id of a MEG channel. */
        MEG_227,					/**< Measurement id of a MEG channel. */
        MEG_228,					/**< Measurement id of a MEG channel. */
        MEG_229,					/**< Measurement id of a MEG channel. */
        MEG_230,					/**< Measurement id of a MEG channel. */
        MEG_231,					/**< Measurement id of a MEG channel. */
        MEG_232,					/**< Measurement id of a MEG channel. */
        MEG_233,					/**< Measurement id of a MEG channel. */
        MEG_234,					/**< Measurement id of a MEG channel. */
        MEG_235,					/**< Measurement id of a MEG channel. */
        MEG_236,					/**< Measurement id of a MEG channel. */
        MEG_237,					/**< Measurement id of a MEG channel. */
        MEG_238,					/**< Measurement id of a MEG channel. */
        MEG_239,					/**< Measurement id of a MEG channel. */
        MEG_240,					/**< Measurement id of a MEG channel. */
        MEG_241,					/**< Measurement id of a MEG channel. */
        MEG_242,					/**< Measurement id of a MEG channel. */
        MEG_243,					/**< Measurement id of a MEG channel. */
        MEG_244,					/**< Measurement id of a MEG channel. */
        MEG_245,					/**< Measurement id of a MEG channel. */
        MEG_246,					/**< Measurement id of a MEG channel. */
        MEG_247,					/**< Measurement id of a MEG channel. */
        MEG_248,					/**< Measurement id of a MEG channel. */
        MEG_249,					/**< Measurement id of a MEG channel. */
        MEG_250,					/**< Measurement id of a MEG channel. */
        MEG_251,					/**< Measurement id of a MEG channel. */
        MEG_252,					/**< Measurement id of a MEG channel. */
        MEG_253,					/**< Measurement id of a MEG channel. */
        MEG_254,					/**< Measurement id of a MEG channel. */
        MEG_255,					/**< Measurement id of a MEG channel. */
        MEG_256,					/**< Measurement id of a MEG channel. */
        MEG_257,					/**< Measurement id of a MEG channel. */
        MEG_258,					/**< Measurement id of a MEG channel. */
        MEG_259,					/**< Measurement id of a MEG channel. */
        MEG_260,					/**< Measurement id of a MEG channel. */
        MEG_261,					/**< Measurement id of a MEG channel. */
        MEG_262,					/**< Measurement id of a MEG channel. */
        MEG_263,					/**< Measurement id of a MEG channel. */
        MEG_264,					/**< Measurement id of a MEG channel. */
        MEG_265,					/**< Measurement id of a MEG channel. */
        MEG_266,					/**< Measurement id of a MEG channel. */
        MEG_267,					/**< Measurement id of a MEG channel. */
        MEG_268,					/**< Measurement id of a MEG channel. */
        MEG_269,					/**< Measurement id of a MEG channel. */
        MEG_270,					/**< Measurement id of a MEG channel. */
        MEG_271,					/**< Measurement id of a MEG channel. */
        MEG_272,					/**< Measurement id of a MEG channel. */
        MEG_273,					/**< Measurement id of a MEG channel. */
        MEG_274,					/**< Measurement id of a MEG channel. */
        MEG_275,					/**< Measurement id of a MEG channel. */
        MEG_276,					/**< Measurement id of a MEG channel. */
        MEG_277,					/**< Measurement id of a MEG channel. */
        MEG_278,					/**< Measurement id of a MEG channel. */
        MEG_279,					/**< Measurement id of a MEG channel. */
        MEG_280,					/**< Measurement id of a MEG channel. */
        MEG_281,					/**< Measurement id of a MEG channel. */
        MEG_282,					/**< Measurement id of a MEG channel. */
        MEG_283,					/**< Measurement id of a MEG channel. */
        MEG_284,					/**< Measurement id of a MEG channel. */
        MEG_285,					/**< Measurement id of a MEG channel. */
        MEG_286,					/**< Measurement id of a MEG channel. */
        MEG_287,					/**< Measurement id of a MEG channel. */
        MEG_288,					/**< Measurement id of a MEG channel. */
        MEG_289,					/**< Measurement id of a MEG channel. */
        MEG_290,					/**< Measurement id of a MEG channel. */
        MEG_291,					/**< Measurement id of a MEG channel. */
        MEG_292,					/**< Measurement id of a MEG channel. */
        MEG_293,					/**< Measurement id of a MEG channel. */
        MEG_294,					/**< Measurement id of a MEG channel. */
        MEG_295,					/**< Measurement id of a MEG channel. */
        MEG_296,					/**< Measurement id of a MEG channel. */
        MEG_297,					/**< Measurement id of a MEG channel. */
        MEG_298,					/**< Measurement id of a MEG channel. */
        MEG_299,					/**< Measurement id of a MEG channel. */
        MEG_300,					/**< Measurement id of a MEG channel. */
        MEG_301,					/**< Measurement id of a MEG channel. */
        MEG_302,					/**< Measurement id of a MEG channel. */
        MEG_303,					/**< Measurement id of a MEG channel. */
        MEG_304,					/**< Measurement id of a MEG channel. */
        MEG_305,					/**< Measurement id of a MEG channel. */
        MEG_306,					/**< Measurement id of a MEG channel. */
        MEG_307,					/**< Measurement id of a MEG channel. */
        MEG_308,					/**< Measurement id of a MEG channel. */
        MEG_309,					/**< Measurement id of a MEG channel. */
        MEG_310,					/**< Measurement id of a MEG channel. */
        MEG_311,					/**< Measurement id of a MEG channel. */
        MEG_312,					/**< Measurement id of a MEG channel. */
        MEG_313,					/**< Measurement id of a MEG channel. */
        MEG_314,					/**< Measurement id of a MEG channel. */
        MEG_315,					/**< Measurement id of a MEG channel. */
        MEG_316,					/**< Measurement id of a MEG channel. */
        MEG_317,					/**< Measurement id of a MEG channel. */
        MEG_318,					/**< Measurement id of a MEG channel. */
        MEG_319,					/**< Measurement id of a MEG channel. */

        STI_0,						/**< Measurement id of a STI channel. */
        STI_1,						/**< Measurement id of a STI channel. */
        STI_2,						/**< Measurement id of a STI channel. */
        STI_3,						/**< Measurement id of a STI channel. */
        STI_4,						/**< Measurement id of a STI channel. */
        STI_5,						/**< Measurement id of a STI channel. */
        STI_6,						/**< Measurement id of a STI channel. */
        STI_7,						/**< Measurement id of a STI channel. */
        STI_8,						/**< Measurement id of a STI channel. */
        STI_9,						/**< Measurement id of a STI channel. */
        STI_10,						/**< Measurement id of a STI channel. */
        STI_11,						/**< Measurement id of a STI channel. */
        STI_12,						/**< Measurement id of a STI channel. */
        STI_13,						/**< Measurement id of a STI channel. */
        STI_14,						/**< Measurement id of a STI channel. */
        STI_15,						/**< Measurement id of a STI channel. */
        STI_16,						/**< Measurement id of a STI channel. */
        STI_17,						/**< Measurement id of a STI channel. */
        STI_18,						/**< Measurement id of a STI channel. */
        STI_19,						/**< Measurement id of a STI channel. */

        EEG_0,						/**< Measurement id of a EEG channel. */
        EEG_1,						/**< Measurement id of a EEG channel. */
        EEG_2,						/**< Measurement id of a EEG channel. */
        EEG_3,						/**< Measurement id of a EEG channel. */
        EEG_4,						/**< Measurement id of a EEG channel. */
        EEG_5,						/**< Measurement id of a EEG channel. */
        EEG_6,						/**< Measurement id of a EEG channel. */
        EEG_7,						/**< Measurement id of a EEG channel. */
        EEG_8,						/**< Measurement id of a EEG channel. */
        EEG_9,						/**< Measurement id of a EEG channel. */
        EEG_10,						/**< Measurement id of a EEG channel. */
        EEG_11,						/**< Measurement id of a EEG channel. */
        EEG_12,						/**< Measurement id of a EEG channel. */
        EEG_13,						/**< Measurement id of a EEG channel. */
        EEG_14,						/**< Measurement id of a EEG channel. */
        EEG_15,						/**< Measurement id of a EEG channel. */
        EEG_16,						/**< Measurement id of a EEG channel. */
        EEG_17,						/**< Measurement id of a EEG channel. */
        EEG_18,						/**< Measurement id of a EEG channel. */
        EEG_19,						/**< Measurement id of a EEG channel. */
        EEG_20,						/**< Measurement id of a EEG channel. */
        EEG_21,						/**< Measurement id of a EEG channel. */
        EEG_22,						/**< Measurement id of a EEG channel. */
        EEG_23,						/**< Measurement id of a EEG channel. */
        EEG_24,						/**< Measurement id of a EEG channel. */
        EEG_25,						/**< Measurement id of a EEG channel. */
        EEG_26,						/**< Measurement id of a EEG channel. */
        EEG_27,						/**< Measurement id of a EEG channel. */
        EEG_28,						/**< Measurement id of a EEG channel. */
        EEG_29,						/**< Measurement id of a EEG channel. */
        EEG_30,						/**< Measurement id of a EEG channel. */
        EEG_31,						/**< Measurement id of a EEG channel. */
        EEG_32,						/**< Measurement id of a EEG channel. */
        EEG_33,						/**< Measurement id of a EEG channel. */
        EEG_34,						/**< Measurement id of a EEG channel. */
        EEG_35,						/**< Measurement id of a EEG channel. */
        EEG_36,						/**< Measurement id of a EEG channel. */
        EEG_37,						/**< Measurement id of a EEG channel. */
        EEG_38,						/**< Measurement id of a EEG channel. */
        EEG_39,						/**< Measurement id of a EEG channel. */
        EEG_40,						/**< Measurement id of a EEG channel. */
        EEG_41,						/**< Measurement id of a EEG channel. */
        EEG_42,						/**< Measurement id of a EEG channel. */
        EEG_43,						/**< Measurement id of a EEG channel. */
        EEG_44,						/**< Measurement id of a EEG channel. */
        EEG_45,						/**< Measurement id of a EEG channel. */
        EEG_46,						/**< Measurement id of a EEG channel. */
        EEG_47,						/**< Measurement id of a EEG channel. */
        EEG_48,						/**< Measurement id of a EEG channel. */
        EEG_49,						/**< Measurement id of a EEG channel. */
        EEG_50,						/**< Measurement id of a EEG channel. */
        EEG_51,						/**< Measurement id of a EEG channel. */
        EEG_52,						/**< Measurement id of a EEG channel. */
        EEG_53,						/**< Measurement id of a EEG channel. */
        EEG_54,						/**< Measurement id of a EEG channel. */
        EEG_55,						/**< Measurement id of a EEG channel. */
        EEG_56,						/**< Measurement id of a EEG channel. */
        EEG_57,						/**< Measurement id of a EEG channel. */
        EEG_58,						/**< Measurement id of a EEG channel. */
        EEG_59,						/**< Measurement id of a EEG channel. */
        EEG_60,						/**< Measurement id of a EEG channel. */
        EEG_61,						/**< Measurement id of a EEG channel. */
        EEG_62,						/**< Measurement id of a EEG channel. */
        EEG_63,						/**< Measurement id of a EEG channel. */
        EEG_64,						/**< Measurement id of a EEG channel. */
        EEG_65,						/**< Measurement id of a EEG channel. */
        EEG_66,						/**< Measurement id of a EEG channel. */
        EEG_67,						/**< Measurement id of a EEG channel. */
        EEG_68,						/**< Measurement id of a EEG channel. */
        EEG_69,						/**< Measurement id of a EEG channel. */
        EEG_70,						/**< Measurement id of a EEG channel. */
        EEG_71,						/**< Measurement id of a EEG channel. */
        EEG_72,						/**< Measurement id of a EEG channel. */
        EEG_73,						/**< Measurement id of a EEG channel. */
        EEG_74,						/**< Measurement id of a EEG channel. */
        EEG_75,						/**< Measurement id of a EEG channel. */
        EEG_76,						/**< Measurement id of a EEG channel. */
        EEG_77,						/**< Measurement id of a EEG channel. */
        EEG_78,						/**< Measurement id of a EEG channel. */
        EEG_79,						/**< Measurement id of a EEG channel. */
        EEG_80,						/**< Measurement id of a EEG channel. */
        EEG_81,						/**< Measurement id of a EEG channel. */
        EEG_82,						/**< Measurement id of a EEG channel. */
        EEG_83,						/**< Measurement id of a EEG channel. */
        EEG_84,						/**< Measurement id of a EEG channel. */
        EEG_85,						/**< Measurement id of a EEG channel. */
        EEG_86,						/**< Measurement id of a EEG channel. */
        EEG_87,						/**< Measurement id of a EEG channel. */
        EEG_88,						/**< Measurement id of a EEG channel. */
        EEG_89,						/**< Measurement id of a EEG channel. */
        EEG_90,						/**< Measurement id of a EEG channel. */
        EEG_91,						/**< Measurement id of a EEG channel. */
        EEG_92,						/**< Measurement id of a EEG channel. */
        EEG_93,						/**< Measurement id of a EEG channel. */
        EEG_94,						/**< Measurement id of a EEG channel. */
        EEG_95,						/**< Measurement id of a EEG channel. */
        EEG_96,						/**< Measurement id of a EEG channel. */
        EEG_97,						/**< Measurement id of a EEG channel. */
        EEG_98,						/**< Measurement id of a EEG channel. */
        EEG_99,						/**< Measurement id of a EEG channel. */
        EEG_100,					/**< Measurement id of a EEG channel. */
        EEG_101,					/**< Measurement id of a EEG channel. */
        EEG_102,					/**< Measurement id of a EEG channel. */
        EEG_103,					/**< Measurement id of a EEG channel. */
        EEG_104,					/**< Measurement id of a EEG channel. */
        EEG_105,					/**< Measurement id of a EEG channel. */
        EEG_106,					/**< Measurement id of a EEG channel. */
        EEG_107,					/**< Measurement id of a EEG channel. */
        EEG_108,					/**< Measurement id of a EEG channel. */
        EEG_109,					/**< Measurement id of a EEG channel. */
        EEG_110,					/**< Measurement id of a EEG channel. */
        EEG_111,					/**< Measurement id of a EEG channel. */
        EEG_112,					/**< Measurement id of a EEG channel. */
        EEG_113,					/**< Measurement id of a EEG channel. */
        EEG_114,					/**< Measurement id of a EEG channel. */
        EEG_115,					/**< Measurement id of a EEG channel. */
        EEG_116,					/**< Measurement id of a EEG channel. */
        EEG_117,					/**< Measurement id of a EEG channel. */
        EEG_118,					/**< Measurement id of a EEG channel. */
        EEG_119,					/**< Measurement id of a EEG channel. */
        EEG_120,					/**< Measurement id of a EEG channel. */
        EEG_121,					/**< Measurement id of a EEG channel. */
        EEG_122,					/**< Measurement id of a EEG channel. */
        EEG_123,					/**< Measurement id of a EEG channel. */
        EEG_124,					/**< Measurement id of a EEG channel. */
        EEG_125,					/**< Measurement id of a EEG channel. */
        EEG_126,					/**< Measurement id of a EEG channel. */
        EEG_127,					/**< Measurement id of a EEG channel. */
        EEG_128,					/**< Measurement id of a EEG channel. */
        EEG_129,					/**< Measurement id of a EEG channel. */
        EEG_130,					/**< Measurement id of a EEG channel. */
        EEG_131,					/**< Measurement id of a EEG channel. */
        EEG_132,					/**< Measurement id of a EEG channel. */
        EEG_133,					/**< Measurement id of a EEG channel. */
        EEG_134,					/**< Measurement id of a EEG channel. */
        EEG_135,					/**< Measurement id of a EEG channel. */
        EEG_136,					/**< Measurement id of a EEG channel. */
        EEG_137,					/**< Measurement id of a EEG channel. */
        EEG_138,					/**< Measurement id of a EEG channel. */
        EEG_139,					/**< Measurement id of a EEG channel. */
        EEG_140,					/**< Measurement id of a EEG channel. */
        EEG_141,					/**< Measurement id of a EEG channel. */
        EEG_142,					/**< Measurement id of a EEG channel. */
        EEG_143,					/**< Measurement id of a EEG channel. */
        EEG_144,					/**< Measurement id of a EEG channel. */
        EEG_145,					/**< Measurement id of a EEG channel. */
        EEG_146,					/**< Measurement id of a EEG channel. */
        EEG_147,					/**< Measurement id of a EEG channel. */
        EEG_148,					/**< Measurement id of a EEG channel. */
        EEG_149,					/**< Measurement id of a EEG channel. */
        EEG_150,					/**< Measurement id of a EEG channel. */
        EEG_151,					/**< Measurement id of a EEG channel. */
        EEG_152,					/**< Measurement id of a EEG channel. */
        EEG_153,					/**< Measurement id of a EEG channel. */
        EEG_154,					/**< Measurement id of a EEG channel. */
        EEG_155,					/**< Measurement id of a EEG channel. */
        EEG_156,					/**< Measurement id of a EEG channel. */
        EEG_157,					/**< Measurement id of a EEG channel. */
        EEG_158,					/**< Measurement id of a EEG channel. */
        EEG_159,					/**< Measurement id of a EEG channel. */
        EEG_160,					/**< Measurement id of a EEG channel. */
        EEG_161,					/**< Measurement id of a EEG channel. */
        EEG_162,					/**< Measurement id of a EEG channel. */
        EEG_163,					/**< Measurement id of a EEG channel. */
        EEG_164,					/**< Measurement id of a EEG channel. */
        EEG_165,					/**< Measurement id of a EEG channel. */
        EEG_166,					/**< Measurement id of a EEG channel. */
        EEG_167,					/**< Measurement id of a EEG channel. */
        EEG_168,					/**< Measurement id of a EEG channel. */
        EEG_169,					/**< Measurement id of a EEG channel. */
        EEG_170,					/**< Measurement id of a EEG channel. */
        EEG_171,					/**< Measurement id of a EEG channel. */
        EEG_172,					/**< Measurement id of a EEG channel. */
        EEG_173,					/**< Measurement id of a EEG channel. */
        EEG_174,					/**< Measurement id of a EEG channel. */
        EEG_175,					/**< Measurement id of a EEG channel. */
        EEG_176,					/**< Measurement id of a EEG channel. */
        EEG_177,					/**< Measurement id of a EEG channel. */
        EEG_178,					/**< Measurement id of a EEG channel. */
        EEG_179,					/**< Measurement id of a EEG channel. */
        EEG_180,					/**< Measurement id of a EEG channel. */
        EEG_181,					/**< Measurement id of a EEG channel. */
        EEG_182,					/**< Measurement id of a EEG channel. */
        EEG_183,					/**< Measurement id of a EEG channel. */
        EEG_184,					/**< Measurement id of a EEG channel. */
        EEG_185,					/**< Measurement id of a EEG channel. */
        EEG_186,					/**< Measurement id of a EEG channel. */
        EEG_187,					/**< Measurement id of a EEG channel. */
        EEG_188,					/**< Measurement id of a EEG channel. */
        EEG_189,					/**< Measurement id of a EEG channel. */
        EEG_190,					/**< Measurement id of a EEG channel. */
        EEG_191,					/**< Measurement id of a EEG channel. */
        EEG_192,					/**< Measurement id of a EEG channel. */
        EEG_193,					/**< Measurement id of a EEG channel. */
        EEG_194,					/**< Measurement id of a EEG channel. */
        EEG_195,					/**< Measurement id of a EEG channel. */
        EEG_196,					/**< Measurement id of a EEG channel. */
        EEG_197,					/**< Measurement id of a EEG channel. */
        EEG_198,					/**< Measurement id of a EEG channel. */
        EEG_199,					/**< Measurement id of a EEG channel. */
        EEG_200,					/**< Measurement id of a EEG channel. */
        EEG_201,					/**< Measurement id of a EEG channel. */
        EEG_202,					/**< Measurement id of a EEG channel. */
        EEG_203,					/**< Measurement id of a EEG channel. */
        EEG_204,					/**< Measurement id of a EEG channel. */
        EEG_205,					/**< Measurement id of a EEG channel. */
        EEG_206,					/**< Measurement id of a EEG channel. */
        EEG_207,					/**< Measurement id of a EEG channel. */
        EEG_208,					/**< Measurement id of a EEG channel. */
        EEG_209,					/**< Measurement id of a EEG channel. */
        EEG_210,					/**< Measurement id of a EEG channel. */
        EEG_211,					/**< Measurement id of a EEG channel. */
        EEG_212,					/**< Measurement id of a EEG channel. */
        EEG_213,					/**< Measurement id of a EEG channel. */
        EEG_214,					/**< Measurement id of a EEG channel. */
        EEG_215,					/**< Measurement id of a EEG channel. */
        EEG_216,					/**< Measurement id of a EEG channel. */
        EEG_217,					/**< Measurement id of a EEG channel. */
        EEG_218,					/**< Measurement id of a EEG channel. */
        EEG_219,					/**< Measurement id of a EEG channel. */
        EEG_220,					/**< Measurement id of a EEG channel. */
        EEG_221,					/**< Measurement id of a EEG channel. */
        EEG_222,					/**< Measurement id of a EEG channel. */
        EEG_223,					/**< Measurement id of a EEG channel. */
        EEG_224,					/**< Measurement id of a EEG channel. */
        EEG_225,					/**< Measurement id of a EEG channel. */
        EEG_226,					/**< Measurement id of a EEG channel. */
        EEG_227,					/**< Measurement id of a EEG channel. */
        EEG_228,					/**< Measurement id of a EEG channel. */
        EEG_229,					/**< Measurement id of a EEG channel. */
        EEG_230,					/**< Measurement id of a EEG channel. */
        EEG_231,					/**< Measurement id of a EEG channel. */
        EEG_232,					/**< Measurement id of a EEG channel. */
        EEG_233,					/**< Measurement id of a EEG channel. */
        EEG_234,					/**< Measurement id of a EEG channel. */
        EEG_235,					/**< Measurement id of a EEG channel. */
        EEG_236,					/**< Measurement id of a EEG channel. */
        EEG_237,					/**< Measurement id of a EEG channel. */
        EEG_238,					/**< Measurement id of a EEG channel. */
        EEG_239,					/**< Measurement id of a EEG channel. */
        EEG_240,					/**< Measurement id of a EEG channel. */
        EEG_241,					/**< Measurement id of a EEG channel. */
        EEG_242,					/**< Measurement id of a EEG channel. */
        EEG_243,					/**< Measurement id of a EEG channel. */
        EEG_244,					/**< Measurement id of a EEG channel. */
        EEG_245,					/**< Measurement id of a EEG channel. */
        EEG_246,					/**< Measurement id of a EEG channel. */
        EEG_247,					/**< Measurement id of a EEG channel. */
        EEG_248,					/**< Measurement id of a EEG channel. */
        EEG_249,					/**< Measurement id of a EEG channel. */
        EEG_250,					/**< Measurement id of a EEG channel. */
        EEG_251,					/**< Measurement id of a EEG channel. */

        EOG_0,						/**< Measurement id of a EOG channel. */
        EOG_1,						/**< Measurement id of a EOG channel. */
        EOG_2,						/**< Measurement id of a EOG channel. */
        EOG_3,						/**< Measurement id of a EOG channel. */
        EOG_4,						/**< Measurement id of a EOG channel. */
        EOG_5,						/**< Measurement id of a EOG channel. */
        EOG_6,						/**< Measurement id of a EOG channel. */
        EOG_7,						/**< Measurement id of a EOG channel. */
        EOG_8,						/**< Measurement id of a EOG channel. */
        EOG_9,						/**< Measurement id of a EOG channel. */
        EOG_10,						/**< Measurement id of a EOG channel. */
        EOG_11,						/**< Measurement id of a EOG channel. */
        EOG_12,						/**< Measurement id of a EOG channel. */
        EOG_13,						/**< Measurement id of a EOG channel. */
        EOG_14,						/**< Measurement id of a EOG channel. */
        EOG_15,						/**< Measurement id of a EOG channel. */
        EOG_16,						/**< Measurement id of a EOG channel. */
        EOG_17,						/**< Measurement id of a EOG channel. */
        EOG_18,						/**< Measurement id of a EOG channel. */
        EOG_19,						/**< Measurement id of a EOG channel. */

        DER_0,						/**< Measurement id of a Derivation channel. */
        DER_1,						/**< Measurement id of a Derivation channel. */
        DER_2,						/**< Measurement id of a Derivation channel. */
        DER_3,						/**< Measurement id of a Derivation channel. */
        DER_4,						/**< Measurement id of a Derivation channel. */
        DER_5,						/**< Measurement id of a Derivation channel. */
        DER_6,						/**< Measurement id of a Derivation channel. */
        DER_7,						/**< Measurement id of a Derivation channel. */
        DER_8,						/**< Measurement id of a Derivation channel. */
        DER_9,						/**< Measurement id of a Derivation channel. */
        DER_10,						/**< Measurement id of a Derivation channel. */
        DER_11,						/**< Measurement id of a Derivation channel. */
        DER_12,						/**< Measurement id of a Derivation channel. */
        DER_13,						/**< Measurement id of a Derivation channel. */
        DER_14,						/**< Measurement id of a Derivation channel. */
        DER_15,						/**< Measurement id of a Derivation channel. */
        DER_16,						/**< Measurement id of a Derivation channel. */
        DER_17,						/**< Measurement id of a Derivation channel. */
        DER_18,						/**< Measurement id of a Derivation channel. */
        DER_19,						/**< Measurement id of a Derivation channel. */
        DER_20,						/**< Measurement id of a Derivation channel. */
        DER_21,						/**< Measurement id of a Derivation channel. */
        DER_22,						/**< Measurement id of a Derivation channel. */
        DER_23,						/**< Measurement id of a Derivation channel. */
        DER_24,						/**< Measurement id of a Derivation channel. */
        DER_25,						/**< Measurement id of a Derivation channel. */
        DER_26,						/**< Measurement id of a Derivation channel. */
        DER_27,						/**< Measurement id of a Derivation channel. */
        DER_28,						/**< Measurement id of a Derivation channel. */
        DER_29,						/**< Measurement id of a Derivation channel. */


        // DummyToolbox
        DUMMYTOOL_OUTPUT = MDL_ID::DUMMYTOOL,	/**< Measurement id of the dummy tool box output channel. */
        DUMMYTOOL_OUTPUT_II,                    /**< Measurement id of the dummy tool box output channel II. */

        // FilterToolbox
        FILTERTOOL_INPUT = MDL_ID::FILTERTOOL,		/**< Measurement id of the filter tool box input channel. */
        FILTERTOOL_OUTPUT,							/**< Measurement id of the filter tool box output channel. */

        // GaborParticleToolbox
        GABORPARTICLETOOL_CURRENT = MDL_ID::GABORPARTICLETOOL,		/**< Measurement id of the gabor particle tool box output channel. */
        GABORPARTICLETOOL_FREQUENCY,								/**< Measurement id of the gabor particle tool box estimated frequency. */
        GABORPARTICLETOOL_FREQUENCY_STD,							/**< Measurement id of the gabor particle tool box estimated standard deviation of the frequency. */
        GABORPARTICLETOOL_SCALE,									/**< Measurement id of the gabor particle tool box estimated scale of the particles. */
        GABORPARTICLETOOL_SCALE_STD,								/**< Measurement id of the gabor particle tool box estimated standard deviation of the scale of the particles. */

        // DummyToolbox
        BRAINMONITOR_OUTPUT = MDL_ID::BRAINMONITOR,		/**< Measurement id of the brain monitor output channel. */


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
