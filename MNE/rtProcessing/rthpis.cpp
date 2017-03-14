//=============================================================================================================
/**
* @file     rthpis.cpp
* @author   Chiran Doshi <chiran.doshi@childrens.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@ntu-ilmenau.de>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*
* @version  1.0
* @date     November, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Chiran Doshi, Lorenz Esch, Limin Sun, and Matti Hamalainen. All rights reserved.
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
* @brief     implementation of the RtHPIS Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rthpis.h"

#include <inverse/hpiFit/hpifit.h>
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;
using namespace IOBUFFER;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtHPIS::RtHPIS(FiffInfo::SPtr p_pFiffInfo, QObject *parent)
: QThread(parent)
, m_pFiffInfo(p_pFiffInfo)
, m_bIsRunning(false)
{
}


//*************************************************************************************************************

RtHPIS::~RtHPIS()
{
    if(this->isRunning()){
        stop();
    }
}


//*************************************************************************************************************

void RtHPIS::append(const MatrixXd &p_DataSegment)
{
    m_mutex.lock();

    if(!m_pRawMatrixBuffer) {
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(8, p_DataSegment.rows(), p_DataSegment.cols()));
    }

    m_pRawMatrixBuffer->push(&p_DataSegment);

    m_mutex.unlock();
}


//*************************************************************************************************************

bool RtHPIS::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning()) {
        QThread::wait();
    }

    m_bIsRunning = true;
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RtHPIS::stop()
{
    m_bIsRunning = false;

    m_pRawMatrixBuffer->releaseFromPop();

    m_pRawMatrixBuffer->clear();

    return true;
}


//*************************************************************************************************************

void RtHPIS::run()
{
//    struct sens sensors;
//    struct coilParam coil;
//    int numCoils = 4;
//    int numCh = m_pFiffInfo->nchan;
//    int samF = m_pFiffInfo->sfreq;
//    int numLoc = 1, numBlock, samLoc; // numLoc : Number of times to localize in a second
//    samLoc = samF/numLoc; // minimum samples required to localize numLoc times in a second
//    Eigen::VectorXd coilfreq(numCoils);
////    coilfreq[0] = 154; coilfreq[1] = 158;coilfreq[2] = 162;coilfreq[3] = 166;
//    coilfreq[0] = 155; coilfreq[1] = 165; coilfreq[2] = 190; coilfreq[3] = 200;

//    qDebug()<< "======= coil driving frequency (Hz)======== ";
//    qDebug() << coilfreq[0] << ", " << coilfreq[1] << ", " << coilfreq[2] << ", " << coilfreq[3];

//    // Initialize HPI coils location and moment
//    coil.pos = Eigen::MatrixXd::Zero(numCoils,3);
//    coil.mom = Eigen::MatrixXd::Zero(numCoils,3);
//    coil.dpfiterror = Eigen::VectorXd::Zero(numCoils);
//    coil.dpfitnumitr = Eigen::VectorXd::Zero(numCoils);

//    // Generate simulated data
//    Eigen::MatrixXd simsig(samLoc,numCoils*2);
//    Eigen::VectorXd time(samLoc);

//    for (int i = 0;i < samLoc;i++) time[i] = i*1.0/samF;

////    std::ofstream outsin;
////    outsin.open ("C:/Users/babyMEG/Desktop/Seok/sin.txt");
////    std::ofstream outcos;
////    outcos.open ("C:/Users/babyMEG/Desktop/Seok/cos.txt");

//    double M_PI = std::atan(1)*4;
//    for(int i=0;i<numCoils;i++) {
//        for(int j=0;j<samLoc;j++) {
//            simsig(j,i) = sin(2*M_PI*coilfreq[i]*time[j]);
//            simsig(j,i+numCoils) = cos(2*M_PI*coilfreq[i]*time[j]);

////            outsin <<simsig(j,i)<<" ";
////            outcos <<simsig(j,i+numCoils) << " ";
//        }
////            outsin <<"\n";
////            outcos <<"\n";
//    }

////    outsin.close();
////    outcos.close();

//    //====== Seok 2016. 3.25 ==========================================
//    // Get the indices of trigger channels
////    QVector<int> trigind(0);
////    for (int i = 0, k = 0 ;i < numCh;i++) {
////        if(m_pFiffInfo->chs[i].coil_type == 3) {
////            k++;
////            if (k >= 9 && k <= 12) {
////                qDebug() << "trigger channel found ..."  << i;
////                trigind.append(i);
////            }
////            if (k >=9 && k <=12) {
////                qDebug() << "HPI trigger channel found ..."  << i;
////                trigind.append(i);
////            }
////        }
////    }
////    qDebug() << "trigind: " << trigind.length();
//    //==================================================================

//    //load polhemus HPI
//    Eigen::MatrixXd headHPI(numCoils,3);

//    // check the m_pFiffInfo->dig information. If dig is empty, set the headHPI is 0;
//    if (m_pFiffInfo->dig.size()>0)
//    {
//        for (int i=0;i<numCoils;i++) {
//            headHPI(i,0) = m_pFiffInfo->dig.at(i+3).r[0];
//            headHPI(i,1) = m_pFiffInfo->dig.at(i+3).r[1];
//            headHPI(i,2) = m_pFiffInfo->dig.at(i+3).r[2];
//        }
//    }
//    else
//    {
//        for (int i=0;i<numCoils;i++) {
//            headHPI(i,0) = 0;headHPI(i,1) = 0;headHPI(i,2) = 0;
//        }
//        qDebug() << "\n \n \n";
//        qDebug()<< "    **********************************************************";
//        qDebug()<< "   *************************************************************";
//        qDebug()<< "  ***************************************************************";
//        qDebug()<< "******************************************************************";
//        qDebug()<< "********    You forget to load polhemus HPI information!   *********";
//        qDebug()<< "***********  Please stop running and load it propoerly!  **********";
//        qDebug()<< "******************************************************************";
//        qDebug()<< " ****************************************************************";
//        qDebug()<< "  **************************************************************";
//        qDebug()<< "   ************************************************************";
//        qDebug()<< "    **********************************************************";
//        qDebug() << "\n \n \n";

//    }

//    Eigen::Matrix4d trans;
//    QVector<MatrixXd> buffer;
//    double phase;

////    qDebug() << "samLoc (1024): " << samLoc;
////    int OUT_FLAG = 0;
////    int OUT_RAW = 0;
////    std::ofstream outinnerdata;
////    outinnerdata.open ("C:/Users/babyMEG/Desktop/Seok/innerdata.txt");
////    std::ofstream outtrigdata;
////    outtrigdata.open ("C:/Users/babyMEG/Desktop/Seok/trigdata.txt");

////    std::ofstream outtopo;
////    outtopo.open ("C:/Users/babyMEG/Desktop/Seok/topo.txt");
////    std::ofstream outamp;
////    outamp.open ("C:/Users/babyMEG/Desktop/Seok/amp.txt");
////    std::ofstream outphase;
////    outphase.open ("C:/Users/babyMEG/Desktop/Seok/phase.txt");
////    std::ofstream outxfm;
////    outxfm.open ("C:/Users/babyMEG/Desktop/Seok/xfm.txt");
////    std::ofstream outcoilp;
////    outcoilp.open ("C:/Users/babyMEG/Desktop/Seok/coilp.txt");
////    std::ofstream outcoilm;
////    outcoilm.open ("C:/Users/babyMEG/Desktop/Seok/coilm.txt");
////    std::ofstream outdpfiterror;
////    outdpfiterror.open ("C:/Users/babyMEG/Desktop/Seok/dpfiterror.txt");
////    std::ofstream outdpfitnumitr;
////    outdpfitnumitr.open ("C:/Users/babyMEG/Desktop/Seok/dpfitnumitr.txt");

//    // --------------------------------------
//    int itimerMatAlloc,itimerLocCoils,itimerTransMulti,itimerPhase,itimerDipFit,itimerCompTrans,itimerBufFull;

//    QElapsedTimer timerBufFull;
//    QElapsedTimer timerMatAlloc;
//    QElapsedTimer timerAll;
//    QElapsedTimer timerLocCoils;
//    QElapsedTimer timerTransMulti;
//    QElapsedTimer timerPhase;
//    QElapsedTimer timerDipFit;

//    QElapsedTimer timerCompTrans;

//    while(m_bIsRunning)
//    {

//        // Get the indices of inner layer channels
//        QVector<int> innerind(0);
//        for (int i = 0;i < numCh;i++) {
//            if(m_pFiffInfo->chs[i].chpos.coil_type == 7002) {
//                // Check if the sensor is bad, if not append to innerind
//                if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) innerind.append(i);
//            }
//        }

//        //qDebug() << "innerind (number of inlayer channels): " << innerind.size();

//        // Initialize inner layer sensors
//        sensors.coilpos = Eigen::MatrixXd::Zero(innerind.size(),3);
//        sensors.coilori = Eigen::MatrixXd::Zero(innerind.size(),3);
//        sensors.tra = Eigen::MatrixXd::Identity(innerind.size(),innerind.size());

//        for(int i=0;i<innerind.size();i++) {
//            sensors.coilpos(i,0) = m_pFiffInfo->chs[innerind.at(i)].chpos.r0[0];
//            sensors.coilpos(i,1) = m_pFiffInfo->chs[innerind.at(i)].chpos.r0[1];
//            sensors.coilpos(i,2) = m_pFiffInfo->chs[innerind.at(i)].chpos.r0[2];
//            sensors.coilori(i,0) = m_pFiffInfo->chs[innerind.at(i)].chpos.ez[0];
//            sensors.coilori(i,1) = m_pFiffInfo->chs[innerind.at(i)].chpos.ez[1];
//            sensors.coilori(i,2) = m_pFiffInfo->chs[innerind.at(i)].chpos.ez[2];
//        }

//        Eigen::MatrixXd topo(innerind.size(),numCoils*2);
//        Eigen::MatrixXd amp(innerind.size(),numCoils);


//        if(m_pRawMatrixBuffer)
//        {
//            //m_mutex.lock();
//            MatrixXd t_mat = m_pRawMatrixBuffer->pop();
//            //m_mutex.unlock();

//            buffer.append(t_mat);

//            timerAll.start();
//            qDebug() << "buffer(size): " << buffer.length();
//            qDebug() << "t_mat(size): " << t_mat.rows() << " x " << t_mat.cols();

//            //If enough data has been stored in the buffer
//            if(buffer.size()*t_mat.cols() >= samLoc)
//            {
//                timerBufFull.start();

//                timerMatAlloc.start();

//                Eigen::MatrixXd alldata(t_mat.rows(),buffer.size()*t_mat.cols());

//                // Concatenate data into a matrix
//                for(int i=0;i<buffer.size();i++)
//                    alldata << buffer[i];

////                // Get the data from inner layer channels
//                Eigen::MatrixXd innerdata(innerind.size(),samLoc);
////                Eigen::MatrixXd trigdata(trigind.size(),samLoc);

//                numBlock = alldata.cols()/samLoc;

//                itimerMatAlloc = timerMatAlloc.elapsed();

//                // Loop for localizing coils

//                timerLocCoils.start();

//                for(int i = 0;i<numBlock;i++) {
//                    for(int j = 0;j < innerind.size();j++){
//                        innerdata.row(j) << alldata.block(innerind[j],i*samLoc,1,samLoc);
////                        if (OUT_RAW == 1) {
////                            for (int k = 0; k < innerdata.cols(); k++)
////                                outinnerdata << innerdata(j,k) << " ";
////                        }
//                   }
////                    for(int j = 0;j < trigind.size();j++){
////                        trigdata.row(j) << alldata.block(trigind[j],i*samLoc,1,samLoc);
////                        if (OUT_RAW == 1) {
////                            for (int k = 0; k < trigdata.cols(); k++)
////                                outtrigdata << trigdata(j,k) << " ";
////                        }
////                   }
////                    if (OUT_RAW == 1) {
////                       outinnerdata << "\n";
////                       outtrigdata << "\n";
////                   }
//                }

//                itimerLocCoils = timerLocCoils.elapsed();

////                    qDebug() << "numBlock: " << numBlock;
////                    qDebug() << "alldata: " << alldata.rows() << " x " << alldata.cols();
////                    qDebug() << "innerdata: " << innerdata.rows() << " x " << innerdata.cols();
////                    qDebug() << "trigdata: " << trigdata.rows() << " x " << trigdata.cols();

//                timerTransMulti.start();
//                    // topo: # of good inner channel x 8
//                    topo = innerdata * pinv(simsig).transpose();

//                itimerTransMulti = timerTransMulti.elapsed();

//                    //topo = innerdata * pinv(trigdata.transpose()).transpose();
//                    //qDebug() << "topo: " << topo.rows() << " " << topo.cols();

////                    for (int i =0; i<numCoils; i++) {
////                        for (int j =0; j< innerind.size(); j++)
////                            outtopo << topo(j,i) << " ";
////
////                        outtopo << "\n";
////                    }


//                // amp: # of good inner channel x 4
//                timerPhase.start();

//                amp = (topo.leftCols(numCoils).array().square() + topo.rightCols(numCoils).array().square()).array().sqrt();
//                //amp = (topo.array().square()).array().sqrt();
//                //qDebug() << "amp: " << amp.rows() << " " << amp.cols();

//                for (int i = 0;i < numCoils;i++) {
//                    for (int j = 0;j < innerind.size();j++) {
//                        phase = atan2(topo(j,i+numCoils),topo(j,i)) * 180/M_PI;
//                        if(phase < 0)
//                            phase = 360 + phase;

//                        if(phase <= 90)
//                            phase = 1;
//                        else if(phase > 90 || phase <= 270)
//                            phase = -1;
//                        else phase = 1;

//                        amp(j,i) = amp(j,i) * phase;

////                            if (OUT_FLAG == 1) {
////                                outamp << amp(j,i) << " ";
////                                outphase << phase << " ";
////                            }
//                    }
////                        if (OUT_FLAG == 1) {
////                            outamp << "\n";
////                            outphase << "\n";
////                        }
//                }

//                itimerPhase = timerPhase.elapsed();

////                    coil.pos(0,0) = 22; coil.pos(0,1) = 60; coil.pos(0,2) = 20;
////                    coil.pos(1,0) = 32; coil.pos(1,1) = 48; coil.pos(1,2) = 34;
////                    coil.pos(2,0) = 35; coil.pos(2,1) = 10; coil.pos(2,2) = 48;
////                    coil.pos(3,0) = 60; coil.pos(3,1) =  4; coil.pos(3,2) = 12;
////                    coil.pos(0,0) = 0; coil.pos(0,1) = 0; coil.pos(0,2) = 0;
////                    coil.pos(1,0) = 0; coil.pos(1,1) = 0; coil.pos(1,2) = 0;
////                    coil.pos(2,0) = 0; coil.pos(2,1) = 0; coil.pos(2,2) = 0;
////                    coil.pos(3,0) = 0; coil.pos(3,1) = 0; coil.pos(3,2) = 0;

//                timerDipFit.start();

//                //coil.pos = Eigen::MatrixXd::Zero(numCoils,3);
//                coil = dipfit(coil, sensors, amp, numCoils);
//                itimerDipFit = timerDipFit.elapsed();


////                    qDebug()<<"HPI head "<<headHPI(0,0)<<" "<<headHPI(0,1)<<" "<<headHPI(0,2);
////                    qDebug()<<"HPI head "<<headHPI(1,0)<<" "<<headHPI(1,1)<<" "<<headHPI(1,2);
////                    qDebug()<<"HPI head "<<headHPI(2,0)<<" "<<headHPI(2,1)<<" "<<headHPI(2,2);
////                    qDebug()<<"HPI head "<<headHPI(3,0)<<" "<<headHPI(3,1)<<" "<<headHPI(3,2);


////                    qDebug()<<"HPI device "<<1e3*coil.pos(0,0)<<" "<<1e3*coil.pos(0,1)<<" "<<1e3*coil.pos(0,2);
////                    qDebug()<<"HPI device "<<1e3*coil.pos(1,0)<<" "<<1e3*coil.pos(1,1)<<" "<<1e3*coil.pos(1,2);
////                    qDebug()<<"HPI device "<<1e3*coil.pos(2,0)<<" "<<1e3*coil.pos(2,1)<<" "<<1e3*coil.pos(2,2);
////                    qDebug()<<"HPI device "<<1e3*coil.pos(3,0)<<" "<<1e3*coil.pos(3,1)<<" "<<1e3*coil.pos(3,2);
////                    qDebug()<<"HPI dpfit error "<<coil.dpfiterror(0) <<" "<<coil.dpfiterror(1) <<" "<<coil.dpfiterror (2)<<" " << coil.dpfiterror(3);

//                    //outcoilp << "   coil position" << "\n";
////                    if (OUT_FLAG == 1) {
////                        outcoilp <<coil.pos(0,0)<<" "<<coil.pos(0,1)<<" "<<coil.pos(0,2) <<"\n";
////                        outcoilp <<coil.pos(1,0)<<" "<<coil.pos(1,1)<<" "<<coil.pos(1,2) <<"\n";
////                        outcoilp <<coil.pos(2,0)<<" "<<coil.pos(2,1)<<" "<<coil.pos(2,2) <<"\n";
////                        outcoilp <<coil.pos(3,0)<<" "<<coil.pos(3,1)<<" "<<coil.pos(3,2) <<"\n";

////                        outcoilm <<coil.mom(0,0)<<" "<<coil.mom(0,1)<<" "<<coil.mom(0,2) <<"\n";
////                        outcoilm <<coil.mom(1,0)<<" "<<coil.mom(1,1)<<" "<<coil.mom(1,2) <<"\n";
////                        outcoilm <<coil.mom(2,0)<<" "<<coil.mom(2,1)<<" "<<coil.mom(2,2) <<"\n";
////                        outcoilm <<coil.mom(3,0)<<" "<<coil.mom(3,1)<<" "<<coil.mom(3,2) <<"\n";

////                        outdpfiterror << coil.dpfiterror(0) <<" "<<coil.dpfiterror(1) <<" "<<coil.dpfiterror(2) <<" " << coil.dpfiterror(3) <<"\n";
////                        outdpfitnumitr << coil.dpfitnumitr(0) <<" "<<coil.dpfitnumitr(1) <<" "<<coil.dpfitnumitr(2) <<" " << coil.dpfitnumitr(3) <<"\n";
////                    }

//                timerCompTrans.start();
//                trans = computeTransformation(coil.pos,headHPI);

//                qDebug()<<"Write to FiffInfo Start";

//                for(int ti =0; ti<4;ti++)
//                    for(int tj=0;tj<4;tj++)
//                        m_pFiffInfo->dev_head_t.trans(ti,tj) = trans(ti,tj);

//                qDebug()<<"Write to FiffInfo End";

//                itimerCompTrans = timerCompTrans.elapsed();

//                    qDebug()<<"**** rotation ------- dev2head transformation ************";
//                    qDebug()<< trans(0,0)<<" "<<trans(0,1)<<" "<<trans(0,2);
//                    qDebug()<< trans(1,0)<<" "<<trans(1,1)<<" "<<trans(1,2);
//                    qDebug()<< trans(2,0)<<" "<<trans(2,1)<<" "<<trans(2,2);
//                    qDebug()<<"**** translation(dx,dy,dz) - dev2head transformation ***********";
//                    qDebug()<< 1e3*trans(0,3)<<" "<<1e3*trans(1,3)<<" "<<1e3*trans(2,3);

///*                     if (OUT_FLAG == 1) {
//                    //outxfm << "   rotation" << "\n";
//                    outxfm << trans(0,0)<<" "<<trans(0,1)<<" "<<trans(0,2) <<" "<< trans(0,3)<<"\n";
//                    outxfm << trans(1,0)<<" "<<trans(1,1)<<" "<<trans(1,2) <<" "<< trans(1,3)<<"\n";
//                    outxfm << trans(2,0)<<" "<<trans(2,1)<<" "<<trans(2,2) <<" "<< trans(2,3)<<"\n";
//                    }
//*/
//                itimerBufFull = timerBufFull.elapsed();

//                buffer.clear();

//                qDebug() << "";
//                qDebug() << "RtHPIS::run() - All" << timerAll.elapsed() << "milliseconds";
//                qDebug() << "";
//                qDebug() << "RtHPIS::run() - itimerMatAlloc" << itimerMatAlloc << "milliseconds";
//                qDebug() << "RtHPIS::run() - itimerLocCoils" << itimerLocCoils << "milliseconds";
//                qDebug() << "RtHPIS::run() - itimerTransMulti" << itimerTransMulti << "milliseconds";
//                qDebug() << "RtHPIS::run() - itimerPhase" << itimerPhase << "milliseconds";
//                qDebug() << "RtHPIS::run() - itimerDipFit" << itimerDipFit << "milliseconds";
//                qDebug() << "RtHPIS::run() - itimerCompTrans" << itimerCompTrans << "milliseconds";
//                qDebug() << "RtHPIS::run() - itimerBufFull" << itimerBufFull << "milliseconds";
//            }


//        }//m_pRawMatrixBuffer
//    }  //End of while statement

//    //m_bIsRunning
////    outinnerdata.close();
////    outtrigdata.close();
////    outtopo.close();
////    outamp.close();
////    outphase.close();
////    outxfm.close();
////    outcoilp.close();
////    outcoilm.close();
////    outdpfiterror.close();
////    outdpfitnumitr.close();

}



/*
Goodness of fit (for MNE HPI GOF)is

g = 1 - sum(e_k^2)/sum(y_k^2)

where

y_k is the measured signal in channel k

e_k = y_k - y’_k, the difference of y_k and the signal predicted by the model
*/
