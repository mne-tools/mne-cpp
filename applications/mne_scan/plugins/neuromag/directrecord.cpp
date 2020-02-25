#include "directrecord.h"

DirectRecord::DirectRecord()
{
}


//=============================================================================================================

bool DirectRecord::start()
{

    // Start threads
    QThread::start();

    return true;
}

//=============================================================================================================

bool DirectRecord::stop()
{
    // Stop threads
    QThread::terminate();
    QThread::wait();

    //Clear Buffers

    return true;
}

//=============================================================================================================

void DirectRecord::run()
{
    while(true)
    {

    }
}
