#ifndef MNE_RT_DATA_CLIENT_H
#define MNE_RT_DATA_CLIENT_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_tag.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpSocket>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

using namespace FIFFLIB;

class MNESHARED_EXPORT MNERtDataClient : public QTcpSocket
{
    Q_OBJECT
public:
    typedef QSharedPointer<MNERtDataClient> SPtr;               /**< Shared pointer type for MNERtDataClient. */
    typedef QSharedPointer<const MNERtDataClient> ConstSPtr;    /**< Const shared pointer type for MNERtDataClient. */

    //=========================================================================================================
    /**
    *
    */
    explicit MNERtDataClient(QObject *parent = 0);

    //=========================================================================================================
    /**
    *
    */
    void connectToHost(QString& p_sRtServerHostName);

    //=========================================================================================================
    /**
    *
    */
    FiffInfo readInfo();

    //=========================================================================================================
    /**
    *
    */
    void readRawBuffer(qint32 p_nChannels, MatrixXf& data, fiff_int_t& kind);

    //=========================================================================================================
    /**
    *
    */
    void setClientAlias(QString p_sAlias)
    {
        FiffStream t_fiffStream(this);
        t_fiffStream.write_rt_command(2, p_sAlias);//MNE_RT.MNE_RT_SET_CLIENT_ALIAS, alias);
        this->flush();

    }


    //=========================================================================================================
    /**
    *
    */
    qint32 getClientId()
    {
        if(m_clientID == -1)
        {
//            sendFiffCommand(1);//MNE_RT.MNE_RT_GET_CLIENT_ID)

            FiffStream t_fiffStream(this);

            QString t_sCommand("");
            t_fiffStream.write_rt_command(1, t_sCommand);


            this->waitForReadyRead(100);
            // ID is send as answer
            FiffTag* t_pTag = NULL;
            FiffTag::read_tag(&t_fiffStream, t_pTag);
            if (t_pTag->kind == FIFF_MNE_RT_CLIENT_ID)
                m_clientID = *t_pTag->toInt();

            delete t_pTag;
        }
        return m_clientID;
    }


private:
    qint32 m_clientID;


signals:
    
public slots:
    
};

} // NAMESPACE

#endif // MNE_RT_DATA_CLIENT_H
