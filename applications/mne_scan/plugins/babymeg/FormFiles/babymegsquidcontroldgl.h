//=============================================================================================================
/**
 * @file     babymegsquidcontroldgl.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2013
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
 * @brief    BabyMEGSQUIDControlDGL class declaration.
 *
 */

#ifndef BABYMEGSQUIDCONTROLDGL_H
#define BABYMEGSQUIDCONTROLDGL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"
#include "globalobj.h"
#include "plotter.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QDialog>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Plotter;
class PlotSettings;

namespace Ui
{
    class BabyMEGSQUIDControlDgl;
}

//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{

//=============================================================================================================
// BABYMEGPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEG;
class PlotSettings;

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

    int BarGraphSelect;

    QVector <double> ParaGraph;
};

//=============================================================================================================
/**
 * The BabyMEGSQUIDControlDgl class provides the SQUID control dialog.
 *
 * @brief The BabyMEGSQUIDControlDgl class provides the SQUID control dialog.
 */
class BABYMEGSHARED_EXPORT BabyMEGSQUIDControlDgl : public QDialog
{
    Q_OBJECT

public:
    explicit BabyMEGSQUIDControlDgl(BabyMEG* p_pBabyMEG,QWidget *parent = 0);
    ~BabyMEGSQUIDControlDgl();
    
    void SendCMD(QString CMDSTR);
    void InitChannels(QString sReply);
    void InitGUIConfig(QString sFLLPara);
    void ReplyCmdProc(QString sReply);
    void UpdateGUI();
    QString GenChnInfo(QString);
    void UpdateInfo(QString newText);
    void ProcCmd(QString cmd, int index, QString Info);
    void InitTuneGraph();
    void TuneGraphDispProc(Eigen::MatrixXf tmp);
    void UpdateParaGraph();
    float mmin(Eigen::MatrixXf tmp,int chan);
    float mmax(Eigen::MatrixXf tmp,int chan);

    void RcvCMDData(QByteArray DATA);

    void Cancel();
    void Init();
    void SyncGUI();

    void Retune();
    void Heat();
    void Atune();
    void Reset();
    void HeatTune();

    void Save();
    void Save1();
    void GroupHeat();
    void Last();
    void Default();

    void TuneCheck();
    void Amp();
    void IntReset();
    void MicroReset();

    void CommType(int index);
    void ChanSele(int index);
    void OperMode(int index);
    void HighPass(int index);
    void LowPass1(int index);
    void SlewSele(int index);
    void PreGaini(int index);
    void PostGain(int index);
    void AutoRest(int index);
    void RestLock(int index);
    void BarGraph(int index);

    void HeatTime();
    void CoolTime();
    void AdOffset();
    void AdjuBias();
    void AdjuModu();

    BabyMEG*                        m_pBabyMEG;
    FLLConfig                       m_FLLConfig;
    GUIStatMachine                  m_GUISM;
    QVector <QGraphicsLineItem*>    PolyLinePtr;
    bool                            initplotflag;
    QVector <QGraphicsRectItem*>    PolyRectPtr;

    PlotSettings                    settings;
    PlotSettings                    settings_tune;

    Plotter*                        d_timeplot;

    int                             TableRows;
    int                             TableCols;
    QList<QString>                  chanNames;

protected:
     virtual void closeEvent( QCloseEvent * event );

private:
    Ui::BabyMEGSQUIDControlDgl*     ui;

signals:
    void SendCMDToMEGSource(QString CMDSTR);
    void inittg();
    void SCStart();
    void SCStop();
};
} //NAMESPACE

#endif // BABYMEGSQUIDCONTROLDGL_H
