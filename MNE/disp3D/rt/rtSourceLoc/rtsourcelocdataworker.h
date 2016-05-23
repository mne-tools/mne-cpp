//=============================================================================================================
/**
* @file     rtsourcelocdataworker.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    RtSourceLocDataWorker class declaration
*
*/

#ifndef RTSOURCELOCDATAWORKER_H
#define RTSOURCELOCDATAWORKER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include "../../disp3D_global.h"
#include "../../helpers/types.h"

#include <disp/helpers/colormap.h>

#include "fs/annotation.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QList>
#include <QThread>
#include <QSharedPointer>
#include <QMutex>
#include <QTime>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace DISPLIB;
using namespace FSLIB;


//=============================================================================================================
/**
* Worker which schedules data with the right timing
*
* @brief Data scheduler
*/
class DISP3DNEWSHARED_EXPORT RtSourceLocDataWorker : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtSourceLocDataWorker> SPtr;            /**< Shared pointer type for RtSourceLocDataWorker class. */
    typedef QSharedPointer<const RtSourceLocDataWorker> ConstSPtr; /**< Const shared pointer type for RtSourceLocDataWorker class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent      The parent of the QObject.
    */
    explicit RtSourceLocDataWorker(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Default destructor.
    */
    ~RtSourceLocDataWorker();

    //=========================================================================================================
    /**
    * Add data which is to be streamed.
    *
    * @param[in] data         The new data.
    */
    void addData(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
    * Clear this worker.
    */
    void clear();

    //=========================================================================================================
    /**
    * Set surface data which the streamed data is plotted on.
    *
    * @param[in] arraySurfaceVertColor  The vertex colors for the surface where the data is to be plotted on.
    * @param[in] vecVertNo              The vertex indexes.
    */
    void setSurfaceData(const QByteArray& arraySurfaceVertColor, const Eigen::VectorXi& vecVertNo);

    //=========================================================================================================
    /**
    * Set annotation data.
    *
    * @param[in] vecLabelIds            The labels ids for each of the surface vertex idx.
    * @param[in] lLabels                The label information.
    */
    void setAnnotationData(const Eigen::VectorXi& vecLabelIds, const QList<FSLIB::Label>& lLabels);

    //=========================================================================================================
    /**
    * Set the number of average to take after emitting the data to the listening threads.
    *
    * @param[in] samples                The new number of averages.
    */
    void setNumberAverages(const int& iNumAvr);

    //=========================================================================================================
    /**
    * Set the length in milli Seconds to wait inbetween data samples.
    *
    * @param[in] iMSec                  The new length in milli Seconds to wait inbetween data samples.
    */
    void setInterval(const int& iMSec);

    //=========================================================================================================
    /**
    * Set the visualization type.
    *
    * @param[in] iVisType               The new visualization type.
    */
    void setVisualizationType(const int& iVisType);

    //=========================================================================================================
    /**
    * Set the type of the colormap.
    *
    * @param[in] sColormapType          The new colormap type.
    */
    void setColormapType(const QString& sColormapType);

    //=========================================================================================================
    /**
    * Set the normalization value.
    *
    * @param[in] dValue                 The new normalization value.
    */
    void setNormalization(const double& dValue);

    //=========================================================================================================
    /**
    * Set the loop functionality on or off.
    *
    * @param[in] looping                The new looping state.
    */
    void setLoop(bool looping);

    //=========================================================================================================
    /**
    * QThread functions
    */
    void stop();
    void start();

protected:
    //=========================================================================================================
    /**
    * QThread functions
    */
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Perfrom the needed visualization type computations, such as smoothing, annoation coliring, etc..
    *
    * @param[in] sourceColorSamples         The color data for the sources.
    *
    * @return                               Returns the final colors in form of a QByteArray.
    */
    QByteArray performVisualizationTypeCalculation(const Eigen::VectorXd& sourceColorSamples);

    //=========================================================================================================
    /**
    * Transform the data sample values to color values.
    *
    * @param[in] data               The data which is to be transformed.
    *
    * @return                       Returns the colors in form of a QByteArray.
    */
    QByteArray transformDataToColor(const Eigen::VectorXd& data);

    QMutex                  m_qMutex;               /**< The thread's mutex. */

    QByteArray              m_arraySurfaceVertColor;/**< The vertex colors for the surface where the data is to be plotted on. */
    QList<Eigen::VectorXd>  m_lData;                /**< List that holds the fiff matrix data <n_channels x n_samples>. */
    VectorXi                m_vecVertNo;            /**< Vector with the source vertx indexes. */

    bool                    m_bIsRunning;           /**< Flag if this thread is running. */
    bool                    m_bIsLooping;           /**< Flag if this thread should repeat sending the same data over and over again. */
    bool                    m_bSurfaceDataIsInit;   /**< Flag if this thread's surface data was initialized. This flag is used to decide whether specific visualization types can be computed. */
    bool                    m_bAnnotationDataIsInit;/**< Flag if this thread's annotation data was initialized. This flag is used to decide whether specific visualization types can be computed. */

    int                     m_iAverageSamples;      /**< Number of average to compute. */
    int                     m_iCurrentSample;       /**< Number of the current sample which is/was streamed. */
    int                     m_iMSecIntervall;       /**< Length in milli Seconds to wait inbetween data samples. */
    int                     m_iVisualizationType;   /**< The visualization type (single vertex, smoothing, annotation based). */

    double                  m_dNormalization;       /**< Normalization value. */
    double                  m_dNormalizationMax;    /**< Value to normalize to. */

    QString                 m_sColormap;            /**< The type of colormap ("Hot", "Hot Negative 1", etc.). */

    QList<FSLIB::Label>     m_lLabels;              /**< The list of current labels. */
    QMap<qint32, qint32>    m_mapLabelIdSources;    /**< The sources mapped to their corresponding labels. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever this item should send a new sample to its listening threads.
    *
    * @param[in] colorSample     The samples data in form of rgb colors as QByteArray.
    */
    void newRtData(QByteArray colorSample);
};

} // NAMESPACE

#endif // RTSOURCELOCDATAWORKER_H
