//=============================================================================================================
/**
 * @file     ecgproducer.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
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
 * @brief    Contains the declaration of the ECGProducer class.
 *
 */

#ifndef ECGPRODUCER_H
#define ECGPRODUCER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/circularbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSIMULATORPLUGIN
//=============================================================================================================

namespace ECGSIMULATORPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class ECGSimulator;


//=============================================================================================================
/**
 * DECLARE CLASS ECGProducer
 *
 * @brief The ECGProducer class provides a ECG data producer for a given sampling rate.
 */
class ECGProducer : public QThread
{
public:
    typedef QSharedPointer<ECGProducer> SPtr;              /**< Shared pointer type for ECGProducer. */
    typedef QSharedPointer<const ECGProducer> ConstSPtr;   /**< Const shared pointer type for ECGProducer. */

    //=========================================================================================================
    /**
    * Constructs a ECGProducer.
    *
    * @param [in] simulator a pointer to the corresponding ECGSimulator.
    * @param [in] buffer_I a pointer to the buffer to which the ECGProducer should write the generated data for ECG I.
    * @param [in] buffer_II a pointer to the buffer to which the ECGProducer should write the generated data for ECG II.
    * @param [in] buffer_III a pointer to the buffer to which the ECGProducer should write the generated data for ECG III.
    */
    ECGProducer(ECGSimulator* simulator, dBuffer::SPtr& buffer_I, dBuffer::SPtr& buffer_II, dBuffer::SPtr& buffer_III);

    //=========================================================================================================
    /**
    * Destroys the ECGProducer.
    */
    ~ECGProducer();

    //=========================================================================================================
    /**
    * Stops the ECGProducer by stopping the producer's thread.
    */
    void stop();

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    ECGSimulator*           m_pECGSimulator;    /**< Holds a pointer to corresponding ECGSimulator.*/
    dBuffer::SPtr           m_pdBuffer_I;       /**< Holds a pointer to the buffer where the simulated data of ECG I should be written to.*/
    dBuffer::SPtr           m_pdBuffer_II;      /**< Holds a pointer to the buffer where the simulated data of ECG II should be written to.*/
    dBuffer::SPtr           m_pdBuffer_III;     /**< Holds a pointer to the buffer where the simulated data of ECG III should be written to.*/
    bool                    m_bIsRunning;       /**< Holds whether ECGProducer is running.*/
};

} // NAMESPACE

#endif // ECGPRODUCER_H
