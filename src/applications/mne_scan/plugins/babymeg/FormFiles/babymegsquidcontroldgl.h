//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymegsquidcontroldgl.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2013
 * @brief    BabyMEGSQUIDControlDGL class declaration.
 */

#ifndef BABYMEGSQUIDCONTROLDGL_H
#define BABYMEGSQUIDCONTROLDGL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../babymeg_global.h"
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
