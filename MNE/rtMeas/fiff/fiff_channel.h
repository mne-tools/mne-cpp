//=============================================================================================================
/**
* @file     fiff_channel.h
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
* @brief    ToDo...
*
*/

#ifndef FIFF_CHANNEL_H
#define FIFF_CHANNEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"
#include "fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFF
//=============================================================================================================

namespace FIFF
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS MEGSimChannel
*
* @brief The MEGSimChannel class provides a MEG channel description.
*/
class RTMEASSHARED_EXPORT FiffChannel {
public:

    //=========================================================================================================
    /**
    * Constructs a FiffChannel.
    */
    FiffChannel();
    //=========================================================================================================
    /**
    * Destroys the FiffChannel.
    */
    virtual ~FiffChannel();

    //=========================================================================================================
    /**
    * Sets the FIFF channel info.
    *
    * @param [in] chInfo the FIFF Channel info Record.
    */
    inline void setFiffChInfo(fiffChInfoRec* chInfo){m_pFiffChInfoRec = chInfo;};

    //=========================================================================================================
    /**
    * Sets whether channel is enabled.
    *
    * @param [in] bEnabled the parameter which enables the channel.
    */
    inline void setEnabled(bool bEnabled){m_bIsEnabled = bEnabled;};
    //=========================================================================================================
    /**
    * Returns whether channel is enabled.
    *
    * @return true when the channel is enabled, false otherwise.
    */
    inline bool isEnabled(){return m_bIsEnabled;};

    //=========================================================================================================
    /**
    * Sets whether channel is visible.
    *
    * @param [in] bVisible the parameter which declares the channel visible.
    */
    inline void setVisible(bool bVisible){m_bIsVisible = bVisible;};
    //=========================================================================================================
    /**
    * Returns whether channel is visible.
    *
    * @return true when the channel is visible, false otherwise.
    */
    inline bool isVisible(){return m_bIsVisible;};

    //=========================================================================================================
    /**
    * Sets the scanning order number of the channel.
    *
    * @param [in] iScanNo the scanning order number which should be set.
    */
    inline void setScanNo(int iScanNo){m_pFiffChInfoRec->scanNo = iScanNo;};
    //=========================================================================================================
    /**
    * Returns the scanning order number of the channel.
    *
    * @return the scanning order number of the channel.
    */
    inline int getScanNo(){return m_pFiffChInfoRec->scanNo;};

    //=========================================================================================================
    /**
    * Sets the logical channel number.
    *
    * @param [in] iLogNo the logical channel number.
    */
    inline void setLogNo(int iLogNo){m_pFiffChInfoRec->logNo = iLogNo;};
    //=========================================================================================================
    /**
    * Returns the logical channel number.
    *
    * @return the logical channel number.
    */
    inline int getLogNo(){return m_pFiffChInfoRec->logNo;};

    //=========================================================================================================
    /**
    * Sets the Kind of the channel.
    *
    * @param [in] iKind the Kind of the channel.
    */
    inline void setKind(int iKind){m_pFiffChInfoRec->kind = iKind;};
    //=========================================================================================================
    /**
    * Returns the channel kind.
    *
    * @return the channel kind.
    */
    inline int getKind(){return m_pFiffChInfoRec->kind;};

    //=========================================================================================================
    /**
    * Sets the Voltmeter range (-1 = auto ranging) of the channel.
    *
    * @param [in] fRange the Voltmeter range (-1 = auto ranging) of the channel.
    */
    inline void setRange(float fRange){m_pFiffChInfoRec->range = fRange;};
    //=========================================================================================================
    /**
    * Returns the Voltmeter range (-1 = auto ranging) of the channel.
    *
    * @return the Voltmeter range (-1 = auto ranging).
    */
    inline float getRange(){return m_pFiffChInfoRec->range;};

    //=========================================================================================================
    /**
    * Sets the Calibration from volts to units used.
    *
    * @param [in] fCal the Calibration from volts to units used.
    */
    inline void setCal(float fCal){m_pFiffChInfoRec->cal = fCal;};
    //=========================================================================================================
    /**
    * Returns the Calibration from volts to units used.
    *
    * @return the Calibration from volts to units used.
    */
    inline float getCal(){return m_pFiffChInfoRec->cal;};

    //=========================================================================================================
    /**
    * Sets the Channel location.
    *
    * @param [in] chpos the Channel location.
    */
    inline void setChPos(fiff_ch_pos_t chpos){m_pFiffChInfoRec->chpos = chpos;};
    //=========================================================================================================
    /**
    * Returns the Channel location.
    *
    * @return the Channel location.
    */
    inline fiff_ch_pos_t getChPos(){return m_pFiffChInfoRec->chpos;};

    //=========================================================================================================
    /**
    * Sets the unit of the channel measurement.
    *
    * @param [in] iUnit the unit of the channel measurement.
    */
    inline void setUnit(int iUnit){m_pFiffChInfoRec->unit = iUnit;};
    //=========================================================================================================
    /**
    * Returns the unit of the channel measurement.
    *
    * @return the unit of the channel measurement.
    */
    inline int getUnit(){return m_pFiffChInfoRec->unit;};

    //=========================================================================================================
    /**
    * Sets the Unit multiplier exponent.
    *
    * @param [in] iUnit_mul the Unit multiplier exponent.
    */
    inline void setUnitMul(int iUnit_mul){m_pFiffChInfoRec->unit_mul = iUnit_mul;};
    //=========================================================================================================
    /**
    * Returns the Unit multiplier exponent.
    *
    * @return the Unit multiplier exponent.
    */
    inline int getUnitMul(){return m_pFiffChInfoRec->unit_mul;};

    //=========================================================================================================
    /**
    * Sets the descriptive name of the channel.
    *
    * @param [in] name the descriptive name of the channel.
    */
    inline void setChName(const char* name){
        #ifdef _WIN32
            strcpy_s ( m_pFiffChInfoRec->ch_name , name);
        #elif defined _WIN64
            strcpy_s ( m_pFiffChInfoRec->ch_name , name);
        #else
            strcpy ( m_pFiffChInfoRec->ch_name , name);
        #endif
    };

    //=========================================================================================================
    /**
    * Returns the descriptive name of the channel.
    *
    * @return the descriptive name of the channel.
    */
    inline QString getChName(){return QString(m_pFiffChInfoRec->ch_name);};

private:

    bool m_bIsEnabled;	/**< Holds whether the channel is enabled.*/
    bool m_bIsVisible;	/**< Holds whether the channel is visible.*/

    fiffChInfoRec* m_pFiffChInfoRec;	/**< Holds channel description record.*/
};




}//NAMESPACE

#endif // FIFF_CHANNEL_H
