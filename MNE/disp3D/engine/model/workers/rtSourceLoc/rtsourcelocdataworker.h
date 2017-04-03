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

#include "../../../../disp3D_global.h"
#include "../../items/common/types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QVector3D>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================
//=========================================================================================================
/**
* The strucut specifing the smoothing operator info.
*/
struct SmoothOperatorInfo {
    VectorXi                vecVertNo;
    SparseMatrix<double>    sparseSmoothMatrix;
    MatrixX3f               matVertPos;
    int                     iDistPow;
    double                  dThresholdDistance;
};

//=========================================================================================================
/**
* The strucut specifing the smoothing vertex info.
*/
struct SmoothVertexInfo {
    int                                 iVertIdx;
    QList<Eigen::Triplet<double> >      lTriplets;
    QList<QVector3D>                    lSourcePos;
    QVector3D                           vVertPos;
    int                                 iDistPow;
    double                              dThresholdDistance;
};

//=========================================================================================================
/**
* The strucut specifing the smoothing visualization info.
*/
struct VisualizationInfo {
    VectorXd                    vSourceColorSamples;
    VectorXi                    vVertNo;
    QList<FSLIB::Label>         lLabels;
    QMap<qint32, qint32>        mapLabelIdSources;
    QMap<int, QVector<int> >    mapVertexNeighbors;
    SparseMatrix<double>        matWDistSmooth;
    double                      dThresholdX;
    double                      dThresholdZ;
    QRgb (*functionHandlerColorMap)(double v);
    MatrixX3f                   matOriginalVertColor;
    MatrixX3f                   matFinalVertColor;
};

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


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
    * Set surface data.
    *
    * @param[in] vecVertNoLeftHemi                  The vertex indexes for the left hemipshere.
    * @param[in] vecVertNoRightHemi                 The vertex indexes for the right hemipshere.
    * @param[in] mapVertexNeighborsLeftHemi         The neighbor vertices for the left hemisphere.
    * @param[in] mapVertexNeighborsRightHemi        The neighbor vertices for the right hemisphere.
    * @param[in] matVertPosLeftHemi                 The surface vertices in 3D space for the left hemisphere.
    * @param[in] matVertPosRightHemi                The surface vertices in 3D space for the right hemisphere.
    */
    void setSurfaceData(const Eigen::VectorXi& vecVertNoLeftHemi,
                        const Eigen::VectorXi& vecVertNoRightHemi,
                        const QMap<int, QVector<int> >& mapVertexNeighborsLeftHemi,
                        const QMap<int, QVector<int> >& mapVertexNeighborsRightHemi,
                        const MatrixX3f& matVertPosLeftHemi,
                        const MatrixX3f& matVertPosRightHemi);

    //=========================================================================================================
    /**
    * Set surface color data which the streamed data is plotted on.
    *
    * @param[in] matSurfaceVertColorLeftHemi      The vertex colors for the left hemipshere surface where the data is to be plotted on.
    * @param[in] matSurfaceVertColorRightHemi     The vertex colors for the right hemipshere surface where the data is to be plotted on.
    */
    void setSurfaceColor(const MatrixX3f &matSurfaceVertColorLeftHemi,
                        const MatrixX3f &matSurfaceVertColorRightHemi);

    //=========================================================================================================
    /**
    * Set annotation data.
    *
    * @param[in] vecLabelIdsLeftHemi        The labels ids for each of the left hemipshere surface vertex idx.
    * @param[in] vecLabelIdsRightHemi       The labels ids for each of the right hemipshere surface vertex idx.
    * @param[in] lLabelsLeftHemi            The label information for the left hemipshere.
    * @param[in] lLabelsRightHemi           The label information for the right hemipshere.
    */
    void setAnnotationData(const Eigen::VectorXi& vecLabelIdsLeftHemi,
                           const Eigen::VectorXi& vecLabelIdsRightHemi,
                           const QList<FSLIB::Label>& lLabelsLeftHemi,
                           const QList<FSLIB::Label>& lLabelsRightHemi);

    //=========================================================================================================
    /**
    * Set the number of average to take after emitting the data to the listening threads.
    *
    * @param[in] samples                The new number of averages.
    */
    void setNumberAverages(int iNumAvr);

    //=========================================================================================================
    /**
    * Set the length in milli Seconds to wait inbetween data samples.
    *
    * @param[in] iMSec                  The new length in milli Seconds to wait inbetween data samples.
    */
    void setInterval(int iMSec);

    //=========================================================================================================
    /**
    * Set the visualization type.
    *
    * @param[in] iVisType               The new visualization type.
    */
    void setVisualizationType(int iVisType);

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
    * @param[in] dValue                 The new threshold values used for normalizing the data.
    */
    void setNormalization(const QVector3D &vecThresholds);

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
    * Perfrom the needed visualization type computations.
    *
    * @param[in] vSourceColorSamples        The color data for the sources.
    *
    * @return                               Returns the final colors in for the left and right hemisphere.
    */
    QPair<MatrixX3f, MatrixX3f> performVisualizationTypeCalculation(const Eigen::VectorXd& vSourceColorSamples);

    //=========================================================================================================
    /**
    * Set the neighboring information for smoothing the activity.
    *
    * @param[in] matVertPosLeftHemi                 The surface vertices in 3D space for the left hemisphere.
    * @param[in] matVertPosRightHemi                The surface vertices in 3D space for the right hemisphere.
    */
    void createSmoothingOperator(const MatrixX3f& matVertPosLeftHemi, const MatrixX3f& matVertPosRightHemi);

    QMutex                  m_qMutex;                           /**< The thread's mutex. */

    QList<Eigen::VectorXd>  m_lData;                            /**< List that holds the fiff matrix data <n_channels x n_samples>. */

    bool                    m_bIsRunning;                       /**< Flag if this thread is running. */
    bool                    m_bIsLooping;                       /**< Flag if this thread should repeat sending the same data over and over again. */
    bool                    m_bSurfaceDataIsInit;               /**< Flag if this thread's surface data was initialized. This flag is used to decide whether specific visualization types can be computed. */
    bool                    m_bAnnotationDataIsInit;            /**< Flag if this thread's annotation data was initialized. This flag is used to decide whether specific visualization types can be computed. */

    int                     m_iAverageSamples;                  /**< Number of average to compute. */
    int                     m_iCurrentSample;                   /**< Number of the current sample which is/was streamed. */
    int                     m_iMSecIntervall;                   /**< Length in milli Seconds to wait inbetween data samples. */
    int                     m_iVisualizationType;               /**< The visualization type (single vertex, smoothing, annotation based). */

    QList<VisualizationInfo>    m_lVisualizationInfo;           /**< The list holding all information needed to do the visualization for both hemispheres (0-left, 1-right). */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever this item should send new colors to its listeners.
    *
    * @param[in] colorPair     The samples data in form of a QPair rgb colors for each (left, right) hemisphere.
    */
    void newRtData(const QPair<MatrixX3f, MatrixX3f>& colorPair);
};

} // NAMESPACE

#endif // RTSOURCELOCDATAWORKER_H
