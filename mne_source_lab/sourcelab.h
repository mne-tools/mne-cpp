#ifndef SOURCELAB_H
#define SOURCELAB_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtdatamanager.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../MNE/fiff/fiff_info.h"


//*************************************************************************************************************
//=============================================================================================================
// Generics INCLUDES
//=============================================================================================================

#include "../MNE/generics/circularmatrixbuffer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>



using namespace IOBuffer;
using namespace FIFFLIB;


class SourceLab : public QThread
{
    Q_OBJECT
public:
    friend class RtDataManager;

    explicit SourceLab(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the SourceLab.
    */
    ~SourceLab();


    virtual bool start();

    virtual bool stop();


protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

signals:
    void closeSourceLab();

public slots:

private:
    RtDataManager*  m_pRtDataManager;

    FiffInfo*       m_pRawInfo;         /**< Holds the fiff raw measurement information. */

    RawMatrixBuffer* m_pRawMatrixBuffer;    /**< The Circular Raw Matrix Buffer. */

    bool    m_bIsRunning;

    QMutex mutex;

};

#endif // SOURCELAB_H
