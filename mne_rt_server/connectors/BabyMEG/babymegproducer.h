//=============================================================================================================
/**
* @file     babymegproducer.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief     declaration of the BabyMEGProducer Class.
*
*/

#ifndef BABYMEGPRODUCER_H
#define BABYMEGPRODUCER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

//#include "circularbuffer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BabyMEGPlugin
//=============================================================================================================

namespace BabyMEGPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEG;


//=============================================================================================================
/**
* DECLARE CLASS BabyMEGProducer
*
* @brief The BabyMEGProducer class provides a ECG data producer for a given sampling rate.
*/
class BabyMEGProducer : public QThread
{
public:

    //=========================================================================================================
    /**
    * Constructs a DataProducer.
    */
    BabyMEGProducer(BabyMEG* p_pBabyMEG);

    //=========================================================================================================
    /**
    * Destroys the DataProducer.
    */
    ~BabyMEGProducer();

    //=========================================================================================================
    /**
    * Stops the DataProducer by stopping the producer's thread.
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
    BabyMEG*  m_pBabyMEG;   /**< Holds a pointer to corresponding FiffSimulator.*/
    bool            m_bIsRunning;       /**< Holds whether ECGProducer is running.*/
};

} // NAMESPACE

#endif // BABYMEGPRODUCER_H
