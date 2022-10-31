//=============================================================================================================
/**
 * @file     babymeginfo.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief     BabyMEGInfo class declaration.
 *
 */

#ifndef BABYMEGINFO_H
#define BABYMEGINFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QQueue>
#include <QStringList>
#include <QWaitCondition>
#include <QMutex>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{

//=============================================================================================================
// BABYMEGPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS BabyMEGInfo
 *
 * @brief The BabyMEGClient class provides a TCP/IP communication between Qt and Labview.
 */
class BABYMEGSHARED_EXPORT BabyMEGInfo : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a BabyMEGInfo class.
     */
    BabyMEGInfo();

    //=========================================================================================================
    /**
     * extract information from string with separate char ":"
     *
     * @param[in] cmdstr     QByteArray contains the header information.
     *
     * @return QStringList   returned information.
     */
    QStringList MGH_LM_Exact_Single_Channel_Info(QByteArray cmdstr);

    //=========================================================================================================
    /**
     * extract information from string with separate char ":"
     *
     * @param[in] cmdstr         QByteArray contains the header information.
     *
     * @return QByteArray        returned information.
     */
    QByteArray MGH_LM_Get_Field(QByteArray cmdstr);
    //=========================================================================================================
    /**
     * extract channel information from string with separate char ":","|",";"
     *
     * @param[in] cmdstr     QByteArray contains the channel information.
     */
    void MGH_LM_Get_Channel_Info(QByteArray cmdstr);
    //=========================================================================================================
    /**
     * Parse the information about header information
     *
     * @param[in] cmdstr     QByteArray contains the header information.
     */
    void MGH_LM_Parse_Para(QByteArray cmdstr);
    //=========================================================================================================
    /**
     * Send data package
     *
     * @param[in] DATA       QByteArray contains MEG data.
     */
    void MGH_LM_Send_DataPackage(QByteArray DATA);
    //=========================================================================================================
    /**
     * Send command reply package
     *
     * @param[in] DATA       QByteArray contains MEG data.
     */
    void MGH_LM_Send_CMDPackage(QByteArray DATA);

    //=========================================================================================================
    /**
     * Parse the information about header information
     *
     * @param[in] cmdstr     QByteArray contains the header information.
     */
    void MGH_LM_Parse_Para_Infg(QByteArray cmdstr);
    //=========================================================================================================
    /**
     * extract channel information from string with separate char ":","|",";"
     *
     * @param[in] cmdstr     QByteArray contains the channel information.
     */
    void MGH_LM_Get_Channel_Infg(QByteArray cmdstr);

    inline FIFFLIB::FiffInfo getFiffInfo() const;

    int         chnNum;
    int         dataLength;
    double      sfreq;
    QStringList lm_ch_names;

    // parameters of single channel
    QStringList lm_ch_scales;
    QStringList lm_ch_pos1;
    QStringList lm_ch_pos2;
    QStringList lm_ch_pos3;
    QStringList lm_ch_pos4;
    QStringList lm_ch_pos5;
    QStringList lm_ch_pos6;
    QStringList lm_ch_pos7;
    QStringList lm_ch_pos8;
    QStringList lm_ch_pos9;
    QStringList lm_ch_pos10;
    QStringList lm_ch_pos11;
    QStringList lm_ch_pos12;
    QStringList lm_ch_coiltype;
    QStringList lm_ch_calicoef;
    QStringList lm_ch_gain;

    //BB_QUEUE
    QQueue<QByteArray>  g_queue;
    int                 g_maxlen;
    QMutex              g_mutex;
    QWaitCondition      g_queueNotFull;
    QWaitCondition      g_queueNotEmpty;

private:
    FIFFLIB::FiffInfo   m_FiffInfo;

signals:
    void fiffInfoAvailable(FIFFLIB::FiffInfo);
    void SendDataPackage(QByteArray DATA);
    void SendCMDPackage(QByteArray DATA);
    void GainInfoUpdate(QStringList);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline FIFFLIB::FiffInfo BabyMEGInfo::getFiffInfo() const
{
    return m_FiffInfo;
}
} // NAMESPACE

#endif // BABYMEGINFO_H
