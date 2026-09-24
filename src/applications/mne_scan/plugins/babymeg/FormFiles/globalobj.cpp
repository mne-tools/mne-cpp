//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     globalobj.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     Oct., 2013
 * @brief     Global variable definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "globalobj.h"

//--- Global Queue for IPC (socket )
QQueue<Eigen::MatrixXf *> g_queue;
int g_maxlen = 40;
QMutex g_mutex;
QWaitCondition g_queueNotFull;
QWaitCondition g_queueNotEmpty;

//--- Global Var for Triggers
PROAvg m_OnlineAvg;
int AvgBufLen;

//--- Global Buffer for Avgeraged Data
QList <Eigen::MatrixXf> AvgBinSum;
QList <int> AvgBufCounts;
QMutex g_mutex_avg;

//--- Global Queue for online averaging
QQueue<Eigen::MatrixXf > g_queue_avg;
int g_maxlen_avg = 40;
QMutex g_mutex_avg1;
QWaitCondition g_queueNotFull_avg;
QWaitCondition g_queueNotEmpty_avg;

//--- Global Queue for online display
QQueue<Eigen::MatrixXf > g_queue_disp;
int g_maxlen_disp = 40;
QMutex g_mutex_disp;
QWaitCondition g_queueNotFull_disp;
QWaitCondition g_queueNotEmpty_disp;

//--- Global Queue for SQUID Control
QQueue<Eigen::MatrixXf > g_queue_squidctrl;
int g_maxlen_squidctrl = 40;
QMutex g_mutex_squidctrl;
QWaitCondition g_queueNotFull_squidctrl;
QWaitCondition g_queueNotEmpty_squidctrl;

//--- Global Queue for File Saving
QQueue<Eigen::MatrixXf > g_queue_filesave;
int g_maxlen_filesave = 40;
QMutex g_mutex_filesave;
QWaitCondition g_queueNotFull_filesave;
QWaitCondition g_queueNotEmpty_filesave;

float  fs;
int    nChn;
float pscale;

//--- Global Filter Setting
gFilter m_gFilter;

