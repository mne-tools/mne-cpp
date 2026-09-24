//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     globalobj.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     Oct., 2013
 * @brief     Global varables definitions.
 */

#ifndef GLOBALOBJ_H
#define GLOBALOBJ_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../babymeg_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore>
#include <QList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// external definition
//=============================================================================================================

extern QQueue <Eigen::MatrixXf *> g_queue;
extern int g_maxlen;
extern QMutex g_mutex;
extern QWaitCondition g_queueNotFull;
extern QWaitCondition g_queueNotEmpty;

//--- Global Queue for online averaging
extern QQueue<Eigen::MatrixXf > g_queue_avg;
extern int g_maxlen_avg;
extern QMutex g_mutex_avg1;
extern QWaitCondition g_queueNotFull_avg;
extern QWaitCondition g_queueNotEmpty_avg;

//--- Global Queue for online display
extern QQueue<Eigen::MatrixXf > g_queue_disp;
extern int g_maxlen_disp;
extern QMutex g_mutex_disp;
extern QWaitCondition g_queueNotFull_disp;
extern QWaitCondition g_queueNotEmpty_disp;

//--- Global Queue for Squid Control
extern QQueue<Eigen::MatrixXf > g_queue_squidctrl;
extern int g_maxlen_squidctrl;
extern QMutex g_mutex_squidctrl;
extern QWaitCondition g_queueNotFull_squidctrl;
extern QWaitCondition g_queueNotEmpty_squidctrl;

//--- Global Queue for File Saving
extern QQueue<Eigen::MatrixXf > g_queue_filesave;
extern int g_maxlen_filesave;
extern QMutex g_mutex_filesave;
extern QWaitCondition g_queueNotFull_filesave;
extern QWaitCondition g_queueNotEmpty_filesave;

//=============================================================================================================
// Structures Definitions
//=============================================================================================================

struct TriggerLine{
    QString TrigChan;
    int NrChn;
};

struct AXPOS {
    int axnum;
    QVector <float> x;
    QVector <float> y;
    float w;
    float h;
    int lanum;
    QList <QString> axlabel;
} ;
struct Trigger{
    int AvgBin;
    QString TrigName;
    QString TrigChan;
    float   TrigVal;
    int     ArtRej;
    int     NrChn;
};
struct PROAvg {
    QString trigfile;
    QString axposfile;
    QString triglinefile;
    int     NumChn;
    AXPOS   m_axpos;
    float   pretime;
    float   posttime;
    int     NumOfAvgBin;
    QList <Trigger> m_trigger;
    int     NumOfTrigLine;
    QList <TriggerLine> m_trigline;
};

extern PROAvg m_OnlineAvg;
extern int AvgBufLen;
extern float  fs;
extern int    nChn;
extern float pscale;

struct gFilter {
    int filtertype;
    float cutfreq1;
    float cutfreq2;
};

extern gFilter m_gFilter;

extern QList <Eigen::MatrixXf> AvgBinSum;
extern QList <int> AvgBufCounts;
extern QMutex g_mutex_avg;

#endif // GLOBALOBJ_H
