//=============================================================================================================
/**
 * @file     globalobj.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     Oct., 2013
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
 * @brief     Global variable definitions.
 *
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

