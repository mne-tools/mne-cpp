//=============================================================================================================
/**
* @file     mnertclientsquidcontroldgl.h
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Limin Sun and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the MneRtClientSQUIDControlDGL class.
*
*/

#ifndef MNERTCLIENTSQUIDCONTROLDGL_H
#define MNERTCLIENTSQUIDCONTROLDGL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_mnertclientsquidcontroldgl.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDialog>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MneRtClientPlugin
//=============================================================================================================

namespace MneRtClientPlugin   //Ui
{
//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
class MneRtClient;

//*************************************************************************************************************
//=============================================================================================================
// Structure definitions
//=============================================================================================================
struct FLLPara{
    int Bias;
    int Mod;
    int Mode;
    int PreGain;
    int PostGain;
    int slew;
    int offset;
    int LoPass;
    int LPBW;
    int HiPass;
    int HPBW;
    int AutoReset;
    int ResetLock;
    QString channame;
};

struct FLLConfig{
    QList < FLLPara > m_Fll;

};

//*************************************************************************************************************
//=============================================================================================================
// GUI Status Machine Structure definitions
//=============================================================================================================
struct GUIStatMachine{
    int CommType;
    int ChannelSel;
    int ChannelStat;
    int OperMode;
    int Retune;
    int HeatThis;
    int Atune;
    int Reset;
    int HeatAndTune;
    int Save;
    int _Save;
    int GroupHeat;
    int Last;
    int Default;

    int HighPass;
    int LowPass;
    int PreGain;
    int PostGain;
    int Slew;
    float HeatTime;
    float CoolTime;

    int AutoRest;
    int ResetLock;
    float offset;
    float bias;
    float modulation;

    int TuneCheck;
    int Amp;
    int IntegratorReset;
    int MicroReset;


};
//=============================================================================================================
/**
* DECLARE CLASS mnertclientSQUIDControlDgl
*
* @brief The mnertclientSQUIDControlDgl class provides the SQUID control dialog.
*/
class mnertclientSQUIDControlDgl : public QDialog
{
    Q_OBJECT


public:
    explicit mnertclientSQUIDControlDgl(MneRtClient* p_pMneRtClient,QWidget *parent = 0);
    ~mnertclientSQUIDControlDgl();
    
private:
    Ui::mnertclientSQUIDControlDgl *ui;

public:
    MneRtClient*   m_pMneRtClient;
    FLLConfig m_FLLConfig;
    GUIStatMachine  m_GUISM;

    void SendRetune();
    void Cancel();
    void SendCMD(QString CMDSTR);
    void Init();
    void InitChannels(QString sReply);
    void InitGUIConfig(QString sFLLPara);
    void ReplyCmdProc(QString sReply);
    void UpdateGUI();
    void SyncGUI();
};

}//namespace
#endif // MNERTCLIENTSQUIDCONTROLDGL_H
