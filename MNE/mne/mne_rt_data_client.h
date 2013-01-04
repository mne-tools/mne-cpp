#ifndef MNE_RT_DATA_CLIENT_H
#define MNE_RT_DATA_CLIENT_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include "fiff/fiff_stream.h"
#include "fiff/fiff_info.h"

#include <QTcpSocket>

using namespace FIFFLIB;

class MNESHARED_EXPORT MNERtDataClient : public QTcpSocket
{
    Q_OBJECT
public:

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
    FiffInfo* readInfo()
    {
        FiffInfo* p_pFiffInfo = new FiffInfo();
        bool t_bReadMeasBlockStart = false;
        bool t_bReadMeasBlockEnd = false;

        FiffStream t_fiffStream(this);
        //
        // Find the start
        //
        FiffTag* t_pTag = NULL;
        while(!t_bReadMeasBlockStart)
        {
            FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
            if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MEAS_INFO)
            {
                printf("FIFF_BLOCK_START FIFFB_MEAS_INFO\n");
                t_bReadMeasBlockStart = true;
            }
        }

        //
        // Parse until the endblock
        //

        bool dev_head_t_read = false;
        bool ctf_head_t_read = false;

        while(!t_bReadMeasBlockEnd)
        {
            FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
            //
            //  megacq parameters
            //
            if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_DACQ_PARS)
            {
                while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_DACQ_PARS)
                {
                    FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                    if(t_pTag->kind == FIFF_DACQ_PARS)
                        p_pFiffInfo->acq_pars = t_pTag->toString();
                    else if(t_pTag->kind == FIFF_DACQ_STIM)
                        p_pFiffInfo->acq_stim = t_pTag->toString();
                }
            }
            //
            //    Coordinate transformations if the HPI result block was not there
            //
            if (t_pTag->kind == FIFF_COORD_TRANS)
            {
                if (!dev_head_t_read)
                {
                    p_pFiffInfo->dev_head_t = t_pTag->toCoordTrans();
                    dev_head_t_read = true;
                }
                else if (!ctf_head_t_read)
                {
                    p_pFiffInfo->ctf_head_t = t_pTag->toCoordTrans();
                    ctf_head_t_read = true;
                }
            }
            //
            //    Polhemus data
            //
            if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_ISOTRAK)
            {
                while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_ISOTRAK)
                {
                    FiffTag::read_rt_tag(&t_fiffStream, t_pTag);

                    if(t_pTag->kind == FIFF_DIG_POINT)
                        p_pFiffInfo->dig.append(t_pTag->toDigPoint());
                }
            }
            //
            //    Projectors
            //
            if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ)
            {
                while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ)
                {
                    FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                    if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_PROJ_ITEM)
                    {
                        FiffProj* proj = NULL;
                        qint32 countProj = p_pFiffInfo->projs.size();
                        while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ_ITEM)
                        {
                            FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                            switch (t_pTag->kind)
                            {
                            case FIFF_NAME: // First proj -> Proj is created
                                proj = new FiffProj();
                                p_pFiffInfo->projs.append(proj);
                                p_pFiffInfo->projs[countProj]->desc = t_pTag->toString();
                                break;
                            case FIFF_PROJ_ITEM_KIND:
                                p_pFiffInfo->projs[countProj]->kind = *(t_pTag->toInt());
                                break;
                            case FIFF_NCHAN: // First data -> FiffNamedMatrix is created
                                p_pFiffInfo->projs[countProj]->data = new FiffNamedMatrix();
                                p_pFiffInfo->projs[countProj]->data->ncol = *(t_pTag->toInt());
                                break;
                            case FIFF_PROJ_ITEM_NVEC:
                                p_pFiffInfo->projs[countProj]->data->nrow = *(t_pTag->toInt());
                                break;
                            case FIFF_MNE_PROJ_ITEM_ACTIVE:
                                p_pFiffInfo->projs[countProj]->active = *(t_pTag->toInt());
                                break;
                            case FIFF_PROJ_ITEM_CH_NAME_LIST:
                                p_pFiffInfo->projs[countProj]->data->col_names = FiffStream::split_name_list(t_pTag->toString());
                                break;
                            case FIFF_PROJ_ITEM_VECTORS:
                                p_pFiffInfo->projs[countProj]->data->data = t_pTag->toFloatMatrix();
                                break;
                            }
                        }
                    }
                }
            }
            //
            //    CTF compensation info
            //
            if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_CTF_COMP)
            {
                while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP)
                {
                    FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                    if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_CTF_COMP_DATA)
                    {
                        FiffCtfComp* comp = NULL;
                        qint32 countComp = p_pFiffInfo->comps.size();
                        while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP_DATA)
                        {
                            FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                            switch (t_pTag->kind)
                            {
                            case FIFF_MNE_CTF_COMP_KIND: //First comp -> create comp
                                comp = new FiffCtfComp();
                                p_pFiffInfo->comps.append(comp);
                                p_pFiffInfo->comps[countComp]->ctfkind = *(t_pTag->toInt());
                                break;
                            case FIFF_MNE_CTF_COMP_CALIBRATED:
                                p_pFiffInfo->comps[countComp]->save_calibrated = *(t_pTag->toInt());
                                break;
//                            case FIFF_MNE_CTF_COMP_DATA:
//                                p_pFiffInfo->comps[countComp]->data = *(t_pTag->toNamedMatrix());
//                                break;
                            }
                        }
                    }
                }
            }
            //
            //    Bad channels
            //
            if(t_pTag->kind == FIFF_BLOCK_START && *(t_pTag->toInt()) == FIFFB_MNE_BAD_CHANNELS)
            {
                while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_BAD_CHANNELS)
                {
                    FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                    if(t_pTag->kind == FIFF_MNE_CH_NAME_LIST)
                        p_pFiffInfo->bads = FiffStream::split_name_list(t_pTag->data());
                }
            }
            //
            //    General
            //
            switch(t_pTag->kind)
            {
            case FIFF_SFREQ:
                p_pFiffInfo->sfreq = *(t_pTag->toFloat());
                break;
            case FIFF_HIGHPASS:
                p_pFiffInfo->highpass = *(t_pTag->toFloat());
                break;
            case FIFF_LOWPASS:
                p_pFiffInfo->lowpass = *(t_pTag->toFloat());
                break;
            case FIFF_NCHAN:
                p_pFiffInfo->nchan = *(t_pTag->toInt());
                break;
            case FIFF_MEAS_DATE:
                p_pFiffInfo->meas_date[0] = t_pTag->toInt()[0];
                p_pFiffInfo->meas_date[1] = t_pTag->toInt()[1];
                break;
            }

            if (t_pTag->kind == FIFF_CH_INFO)
                p_pFiffInfo->chs.append(t_pTag->toChInfo());

            // END MEAS
            if(t_pTag->kind == FIFF_BLOCK_END && *t_pTag->toInt() == FIFFB_MEAS_INFO)
            {
                printf("FIFF_BLOCK_END FIFFB_MEAS_INFO\n");
                t_bReadMeasBlockEnd = true;
            }
        }
        return p_pFiffInfo;
    }


    //=========================================================================================================
    /**
    *
    */
    void readRawBuffer(qint32 p_nChannels, MatrixXf& data, fiff_int_t& kind)
    {
//        data = [];

        FiffStream t_fiffStream(this);
        //
        // Find the start
        //
        FiffTag* t_pTag = NULL;

        FiffTag::read_rt_tag(&t_fiffStream, t_pTag);

        kind = t_pTag->kind;

        if(kind == FIFF_DATA_BUFFER)
        {
            qint32 nSamples = (t_pTag->size()/4)/p_nChannels;
            data = MatrixXf(Map< MatrixXf >(t_pTag->toFloat(), p_nChannels, nSamples));
        }
//        else
//            data = tag.data;
    }


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

#endif // MNE_RT_DATA_CLIENT_H
