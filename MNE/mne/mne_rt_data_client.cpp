#include "mne_rt_data_client.h"


using namespace MNELIB;


MNERtDataClient::MNERtDataClient(QObject *parent)
: QTcpSocket(parent)
, m_clientID(-1)
{
    getClientId();
}


//*************************************************************************************************************

void MNERtDataClient::connectToHost(QString& p_sRtServerHostName)
{
    QTcpSocket::connectToHost(p_sRtServerHostName, 4218);
}


//*************************************************************************************************************

FiffInfo MNERtDataClient::readInfo()
{
    FiffInfo p_FiffInfo;
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
                    p_FiffInfo.acq_pars = t_pTag->toString();
                else if(t_pTag->kind == FIFF_DACQ_STIM)
                    p_FiffInfo.acq_stim = t_pTag->toString();
            }
        }
        //
        //    Coordinate transformations if the HPI result block was not there
        //
        if (t_pTag->kind == FIFF_COORD_TRANS)
        {
            if (!dev_head_t_read)
            {
                p_FiffInfo.dev_head_t = t_pTag->toCoordTrans();
                dev_head_t_read = true;
            }
            else if (!ctf_head_t_read)
            {
                p_FiffInfo.ctf_head_t = t_pTag->toCoordTrans();
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
                    p_FiffInfo.dig.append(t_pTag->toDigPoint());
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
                    FiffProj proj;
                    qint32 countProj = p_FiffInfo.projs.size();
                    while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_PROJ_ITEM)
                    {
                        FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                        switch (t_pTag->kind)
                        {
                        case FIFF_NAME: // First proj -> Proj is created
                            proj = FiffProj();
                            p_FiffInfo.projs.append(proj);
                            p_FiffInfo.projs[countProj].desc = t_pTag->toString();
                            break;
                        case FIFF_PROJ_ITEM_KIND:
                            p_FiffInfo.projs[countProj].kind = *(t_pTag->toInt());
                            break;
                        case FIFF_NCHAN: // First data -> FiffNamedMatrix is created
//                                p_FiffInfo.projs[countProj].data = FiffNamedMatrix();//obsolete
                            p_FiffInfo.projs[countProj].data->ncol = *(t_pTag->toInt());
                            break;
                        case FIFF_PROJ_ITEM_NVEC:
                            p_FiffInfo.projs[countProj].data->nrow = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_PROJ_ITEM_ACTIVE:
                            p_FiffInfo.projs[countProj].active = *(t_pTag->toInt());
                            break;
                        case FIFF_PROJ_ITEM_CH_NAME_LIST:
                            p_FiffInfo.projs[countProj].data->col_names = FiffStream::split_name_list(t_pTag->toString());
                            break;
                        case FIFF_PROJ_ITEM_VECTORS:
                            p_FiffInfo.projs[countProj].data->data = t_pTag->toFloatMatrix();
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
                    FiffCtfComp comp;
                    qint32 countComp = p_FiffInfo.comps.size();
                    while(t_pTag->kind != FIFF_BLOCK_END || *(t_pTag->toInt()) != FIFFB_MNE_CTF_COMP_DATA)
                    {
                        FiffTag::read_rt_tag(&t_fiffStream, t_pTag);
                        switch (t_pTag->kind)
                        {
                        case FIFF_MNE_CTF_COMP_KIND: //First comp -> create comp
                            comp = FiffCtfComp();
                            p_FiffInfo.comps.append(comp);
                            p_FiffInfo.comps[countComp].ctfkind = *(t_pTag->toInt());
                            break;
                        case FIFF_MNE_CTF_COMP_CALIBRATED:
                            p_FiffInfo.comps[countComp].save_calibrated = *(t_pTag->toInt());
                            break;
//                            case FIFF_MNE_CTF_COMP_DATA:
//                                p_FiffInfo.comps[countComp]->data = *(t_pTag->toNamedMatrix());
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
                    p_FiffInfo.bads = FiffStream::split_name_list(t_pTag->data());
            }
        }
        //
        //    General
        //
        switch(t_pTag->kind)
        {
        case FIFF_SFREQ:
            p_FiffInfo.sfreq = *(t_pTag->toFloat());
            break;
        case FIFF_HIGHPASS:
            p_FiffInfo.highpass = *(t_pTag->toFloat());
            break;
        case FIFF_LOWPASS:
            p_FiffInfo.lowpass = *(t_pTag->toFloat());
            break;
        case FIFF_NCHAN:
            p_FiffInfo.nchan = *(t_pTag->toInt());
            break;
        case FIFF_MEAS_DATE:
            p_FiffInfo.meas_date[0] = t_pTag->toInt()[0];
            p_FiffInfo.meas_date[1] = t_pTag->toInt()[1];
            break;
        }

        if (t_pTag->kind == FIFF_CH_INFO)
            p_FiffInfo.chs.append(t_pTag->toChInfo());

        // END MEAS
        if(t_pTag->kind == FIFF_BLOCK_END && *t_pTag->toInt() == FIFFB_MEAS_INFO)
        {
            printf("FIFF_BLOCK_END FIFFB_MEAS_INFO\n");
            t_bReadMeasBlockEnd = true;
        }
    }
    return p_FiffInfo;
}


//*************************************************************************************************************

void MNERtDataClient::readRawBuffer(qint32 p_nChannels, MatrixXf& data, fiff_int_t& kind)
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
