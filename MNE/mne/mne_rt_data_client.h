#ifndef MNE_RT_DATA_CLIENT_H
#define MNE_RT_DATA_CLIENT_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"


#include "../fiff/fiff_stream.h"

#include "../fiff/fiff_info.h"



#include <QTcpSocket>



using namespace FIFFLIB;


class MNESHARED_EXPORT MNERtDataClient : public QTcpSocket
{
    Q_OBJECT
public:
    explicit MNERtDataClient(QObject *parent = 0);

    void connectToHost(QString& p_sRtServerHostName);


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










//            % =================================================================
//            %% readRawBuffer
//            function [kind, data] = readRawBuffer(obj, p_nChannels)
//                %
//                % [kind, data] = readRawBuffer(obj, p_nChannels)
//                %
//                % reads a raw buffer
//                %
//                % p_nChannels - number of channels to reshape the incomming
//                %               float array
//                %
//                % kind        - FIFF_DATA_BUFFER ->
//                %               FIFF_BLOCK_START -> data = FIFFB_RAW_DATA
//                %               FIFF_BLOCK_END -> data = FIFFB_RAW_DATA
//                % data        - the read buffer

//                %
//                %   Author : Christoph Dinh, Matti Hamalainen, MGH Martinos Center
//                %   License : BSD 3-clause
//                %

//                import java.net.Socket
//                import java.io.*

//                global FIFF;
//                if isempty(FIFF)
//                    FIFF = fiff_define_constants();
//                end

//                data = [];
//                kind = [];

//                if ~isempty(obj.m_DataInputStream)

//                    bytes_available = obj.m_DataInputStream.available;
//                    % Wait for incomming bytes
//                    while(bytes_available == 0)
//                        bytes_available = obj.m_DataInputStream.available;
//                    end

//                    tag = mne_rt_data_client.read_tag(obj.m_DataInputStream);

//                    kind = tag.kind;

//                    if(tag.kind == FIFF.FIFF_DATA_BUFFER)
//                        nSamples = length(tag.data)/p_nChannels;
//                        data = reshape(tag.data, p_nChannels, nSamples);
//                    else
//                        data = tag.data;
//                    end
//                else
//                    error('mne_rt_data_client: no available TcpSocket, call init to establish a connection.');
//                end
//            end


    void setClientAlias(QString p_sAlias)
    {
        FiffStream t_fiffStream(this);
        t_fiffStream.write_rt_command(2, p_sAlias);//MNE_RT.MNE_RT_SET_CLIENT_ALIAS, alias);
        this->flush();

    }


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




//            % =================================================================
//            %% read_tag
//            function [tag] = read_tag(p_DataInputStream)
//                %
//                % [tag] = read_tag(p_DataInputStream)
//                %
//                % reads a fiff encoded real-time data stream
//                %
//                % p_DataInputStream - open data stream
//                %

//                %
//                %   Author : Christoph Dinh, Matti Hamalainen, MGH Martinos Center
//                %   License : BSD 3-clause
//                %
//                import java.net.Socket
//                import java.io.*

//                me='MNE_RT_DATA_CLIENT:read_tag';

//                %
//                % read the tag info
//                %
//                tagInfo = mne_rt_data_client.read_tag_info(p_DataInputStream);

//                %
//                % read the tag data
//                %
//                tag = mne_rt_data_client.read_tag_data(p_DataInputStream, tagInfo);
//            end

//            % =================================================================
//            %% read_tag_data
//            function [tag] = read_tag_data(p_DataInputStream, p_tagInfo, pos)
//            %
//            % [tag] = read_tag_data(p_dInputStream, pos)
//            %
//            % Reads the tag data from a fif stream.
//            % if pos is not provided, reading starts from the current stream position
//            %
//            % p_DataInputStream - the open data stream
//            % p_tagInfo         - the tag info
//            % pos               - number of bytes to be skipped
//            %

//            %
//            %   Author : Christoph Dinh, Matti Hamalainen, MGH Martinos Center
//            %   License : BSD 3-clause
//            %
//                import java.net.Socket
//                import java.io.*

//                global FIFF;
//                if isempty(FIFF)
//                    FIFF = fiff_define_constants();
//                end

//                me='MNE_RT_DATA_CLIENT:read_tag_data';

//                if nargin == 3
//                    p_DataInputStream.skipBytes(pos);
//                elseif nargin ~= 2
//                    error(me,'Incorrect number of arguments');
//                end

//                tag = p_tagInfo;

//                %
//                %   The magic hexadecimal values
//                %
//                is_matrix           = 4294901760; % ffff0000
//                matrix_coding_dense = 16384;      % 4000
//                matrix_coding_CCS   = 16400;      % 4010
//                matrix_coding_RCS   = 16416;      % 4020
//                data_type           = 65535;      % ffff
//                %
//                if tag.size > 0
//                    matrix_coding = bitand(is_matrix,tag.type);
//                    if matrix_coding ~= 0
//                        matrix_coding = bitshift(matrix_coding,-16);
//                        %
//                        %   Matrices
//                        %
//                        if matrix_coding == matrix_coding_dense
//                            %
//                            % Find dimensions and return to the beginning of tag data
//                            %

//    % Check can't be done in real-time --> moved to the end for reshape
//    %                         pos = ftell(fid);
//    %                         fseek(fid,tag.size-4,'cof');
//    %                         ndim = fread(fid,1,'int32');
//    %                         fseek(fid,-(ndim+1)*4,'cof');
//    %                         dims = fread(fid,ndim,'int32');
//    %                         %
//    %                         % Back to where the data start
//    %                         %
//    %                         fseek(fid,pos,'bof');

//                            matrix_type = bitand(data_type,tag.type);

//                            el_size = tag.size - 3*4; % 3*4 --> case 2D matrix; ToDo calculate el_size through

//                            switch matrix_type
//    %                             case FIFF.FIFFT_INT
//    %                                 tag.data = zeros(el_size/4, 1);
//    %                                 for i = 1:el_size/4
//    %                                     tag.data(i) = p_DataInputStream.readInt;%idata = fread(fid,dims(1)*dims(2),'int32=>int32');
//    %                                 end
//    %                             case FIFF.FIFFT_JULIAN
//    %                                 tag.data = zeros(el_size/4, 1);
//    %                                 for i = 1:el_size/4
//    %                                     tag.data(i) = p_DataInputStream.readInt;%idata = fread(fid,dims(1)*dims(2),'int32=>int32');
//    %                                 end
//                                case FIFF.FIFFT_FLOAT
//                                    t_MNERTBufferReader = mne_rt_buffer_reader(p_DataInputStream);
//                                    tmp = t_MNERTBufferReader.readBuffer(el_size);
//                                    tag.data = typecast(tmp, 'single');%fdata = fread(fid,dims(1)*dims(2),'single=>double');
//                                    tag.data = swapbytes(tag.data);
//                                otherwise
//                                    error(me,'Cannot handle a matrix of type %d yet',matrix_type)
//                            end

//                            % ToDo consider 3D case --> do that by using tag->size
//                            dims = zeros(1, 2);

//                            dims(1) = p_DataInputStream.readInt;
//                            dims(2) = p_DataInputStream.readInt;

//                            ndim = p_DataInputStream.readInt;

//                            tag.data = reshape(tag.data,dims(1),dims(2))';
//                        else
//                            error(me,'Cannot handle other than dense or sparse matrices yet')
//                        end
//                    else
//                        %
//                        %   All other data types
//                        %
//                        switch tag.type
//                            %
//                            %   Simple types
//                            %
//                            case FIFF.FIFFT_INT
//    %                             tag.data = zeros(tag.size/4, 1);
//    %                             for i = 1:tag.size/4
//    %                                 tag.data(i) = p_DataInputStream.readInt;%fread(fid,tag.size/4,'int32=>int32');
//    %                             end
//                                t_MNERTBufferReader = mne_rt_buffer_reader(p_DataInputStream);
//                                tmp = t_MNERTBufferReader.readBuffer(tag.size);
//                                tag.data = typecast(tmp, 'int32');%fread(fid,tag.size/4,'int32=>int32');
//                                tag.data = swapbytes(tag.data);
//                            case FIFF.FIFFT_FLOAT
//                                t_MNERTBufferReader = mne_rt_buffer_reader(p_DataInputStream);
//                                tmp = t_MNERTBufferReader.readBuffer(tag.size);
//                                tag.data = typecast(tmp, 'single');%fread(fid,tag.size/4,'single=>double');
//                                tag.data = swapbytes(tag.data);
//                            case FIFF.FIFFT_STRING
//                                t_MNERTBufferReader = mne_rt_buffer_reader(p_DataInputStream);
//                                tag.data = t_MNERTBufferReader.readBuffer(tag.size);%fread(fid,tag.size,'uint8=>char')';
//                                tag.data = char(tag.data);
//                            case FIFF.FIFFT_ID_STRUCT
//                                tag.data.version = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.machid = zeros(2,1);
//                                tag.data.machid(1)  = p_DataInputStream.readInt;%fread(fid,2,'int32=>int32');
//                                tag.data.machid(2)  = p_DataInputStream.readInt;
//                                tag.data.secs    = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.usecs   = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                            case FIFF.FIFFT_DIG_POINT_STRUCT
//                                tag.data.kind    = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.ident   = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.r = zeros(3,1);
//                                for i = 1:3
//                                    tag.data.r(i)	= p_DataInputStream.readFloat;%fread(fid,3,'single=>single');
//                                end
//                                tag.data.coord_frame = 0;
//                            case FIFF.FIFFT_COORD_TRANS_STRUCT
//                                tag.data.from = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.to   = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                rot = zeros(9,1);
//                                for i = 1:9
//                                    rot(i) = p_DataInputStream.readFloat;%fread(fid,9,'single=>double');
//                                end
//                                rot = reshape(rot,3,3)';
//                                move = zeros(3,1);
//                                for i = 1:3
//                                    move(i) = p_DataInputStream.readFloat;%fread(fid,3,'single=>double');
//                                end
//                                tag.data.trans = [ rot move ; [ 0  0 0 1 ]];
//                                %
//                                % Skip over the inverse transformation
//                                % It is easier to just use inverse of trans in Matlab
//                                %
//                                for i = 1:12 %fseek(fid,12*4,'cof');
//                                    p_DataInputStream.readFloat;
//                                end
//                            case FIFF.FIFFT_CH_INFO_STRUCT
//                                tag.data.scanno    = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.logno     = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.kind      = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.range     = p_DataInputStream.readFloat;%fread(fid,1,'single=>double');
//                                tag.data.cal       = p_DataInputStream.readFloat;%fread(fid,1,'single=>double');
//                                tag.data.coil_type = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                %
//                                %   Read the coil coordinate system definition
//                                %
//                                tag.data.loc = zeros(12,1);
//                                for i = 1:12
//                                    tag.data.loc(i) = p_DataInputStream.readFloat;%fread(fid,12,'single=>double');
//                                end
//                                tag.data.coil_trans  = [];
//                                tag.data.eeg_loc     = [];
//                                tag.data.coord_frame = FIFF.FIFFV_COORD_UNKNOWN;
//                                %
//                                %   Convert loc into a more useful format
//                                %
//                                loc = tag.data.loc;
//                                if tag.data.kind == FIFF.FIFFV_MEG_CH || tag.data.kind == FIFF.FIFFV_REF_MEG_CH
//                                    tag.data.coil_trans  = [ [ loc(4:6) loc(7:9) loc(10:12) loc(1:3) ] ; [ 0 0 0 1 ] ];
//                                    tag.data.coord_frame = FIFF.FIFFV_COORD_DEVICE;
//                                elseif tag.data.kind == FIFF.FIFFV_EEG_CH
//                                    if norm(loc(4:6)) > 0
//                                        tag.data.eeg_loc     = [ loc(1:3) loc(4:6) ];
//                                    else
//                                        tag.data.eeg_loc = [ loc(1:3) ];
//                                    end
//                                    tag.data.coord_frame = FIFF.FIFFV_COORD_HEAD;
//                                end
//                                %
//                                %   Unit and exponent
//                                %
//                                tag.data.unit     = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                tag.data.unit_mul = p_DataInputStream.readInt;%fread(fid,1,'int32=>int32');
//                                %
//                                %   Handle the channel name
//                                %
//                                ch_name = zeros(1, 16, 'uint8');
//                                for i = 1:16
//                                    ch_name(i) = p_DataInputStream.readByte;
//                                end
//                                ch_name   = char(ch_name);
//                                %
//                                % Omit nulls
//                                %
//                                len = 16;
//                                for k = 1:16
//                                    if ch_name(k) == 0
//                                        len = k-1;
//                                        break
//                                    end
//                                end
//                                tag.data.ch_name = ch_name(1:len);
//                            otherwise
//                                error(me,'Unimplemented tag data type %d',tag.type);

//                        end
//                    end
//                end

//                % if tag.next ~= FIFF.FIFFV_NEXT_SEQ
//                %     fseek(fid,tag.next,'bof');
//                % end

//                return;
//            end

//            % =================================================================
//            %% read_tag_info
//            function [tag] = read_tag_info(p_DataInputStream, pos)
//            %
//            % [tag] = read_tag_info(p_inStream, pos)
//            %
//            % Read tag info from a fif stream.
//            % if pos is not provided, reading starts from the current stream position
//            %
//            % p_DataInputStream - the open data stream
//            % pos               - number of bytes to be skipped
//            %

//            %
//            %   Author : Christoph Dinh, Matti Hamalainen, MGH Martinos Center
//            %   License : BSD 3-clause
//            %
//                import java.net.Socket
//                import java.io.*

//                global FIFF;
//                if isempty(FIFF)
//                    FIFF = fiff_define_constants();
//                end

//                me='MNE_RT_DATA_CLIENT:read_tag_info';

//                if nargin == 2
//                    p_DataInputStream.skipBytes(pos);
//                elseif nargin ~= 1
//                    error(me,'Incorrect number of arguments');
//                end

//    %             while true
//    %                 bytes_available = p_inStream.available;
//    %                 if(bytes_available >= 16)
//                tag.kind = p_DataInputStream.readInt;
//                tag.type = p_DataInputStream.readInt;
//                tag.size = p_DataInputStream.readInt;
//                tag.next = p_DataInputStream.readInt;
//    %                     break;
//    %                 end
//    %                 % pause 100ms before retrying
//    %                 pause(0.1);
//    %             end

//                return;
//            end
//        end
//    end



















private:
    qint32 m_clientID;






signals:
    
public slots:
    
};

#endif // MNE_RT_DATA_CLIENT_H
