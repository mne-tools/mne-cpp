#ifndef RTINV_H
#define RTINV_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtinv_global.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <mne/mne_forwardsolution.h>
#include <mne/mne_inverse_operator.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RtInvLIB
//=============================================================================================================

namespace RTINVLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;


//=============================================================================================================
/**
* DECLARE CLASS RtInv
*
* @brief The RtInv class provides...
*/
class RTINVSHARED_EXPORT RtInv : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtInv> SPtr;             /**< Shared pointer type for RtInv. */
    typedef QSharedPointer<const RtInv> ConstSPtr;  /**< Const shared pointer type for RtInv. */

    explicit RtInv(FiffInfo::SPtr p_pFiffInfo, MNEForwardSolution::SPtr p_pFwd, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the CovRt.
    */
    ~RtInv();

    void receiveNoiseCov(MatrixXf p_noiseCov);

    //=========================================================================================================
    /**
    * Stops the RtInv by stopping the producer's thread.
    *
    * @return true if succeeded, false otherwise
    */
    virtual bool stop();

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    QMutex      mutex;
    bool        m_bIsRunning;           /**< Holds whether RtInv is running.*/

    MatrixXf    m_noiseCov;

    FiffInfo::SPtr      m_pFiffInfo;
    MNEForwardSolution::SPtr m_pFwd;

signals:
    void invOperatorAvailable(MNELIB::MNEInverseOperator);
};

} // NAMESPACE

#ifndef metatype_mneinverseoperator
#define metatype_mneinverseoperator
Q_DECLARE_METATYPE(MNELIB::MNEInverseOperator);
#endif

#endif // RTINV_H
