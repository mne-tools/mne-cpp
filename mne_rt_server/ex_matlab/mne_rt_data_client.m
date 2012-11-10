classdef mne_rt_data_client < mne_rt_client
    %MNE_RT_DATA_CLIENT Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        m_clientID = -1;
    end
    
    methods
        
        % =================================================================
        %% mne_rt_data_client
        function obj = mne_rt_data_client(host, port, numOfRetries)
            if (nargin < 3)
                numOfRetries = 20; % set to -1 for infinite
            end
            obj = obj@mne_rt_client(host, port, numOfRetries);%Superclass call
            obj.getClientId();
        end % mne_rt_data_client
        
        
        % =================================================================
        %% readInfo
        function [info] = readInfo(obj)
            import java.net.Socket
            import java.io.*
            
            global FIFF;
            if isempty(FIFF)
                FIFF = fiff_define_constants();
            end
            
            if ~isempty(obj.m_TcpSocket)
                % get a buffered data input stream from the socket
                t_inStream   = obj.m_TcpSocket.getInputStream;

                t_bReadMeasBlockStart = false;
                t_bReadMeasBlockEnd = false;
                %
                % Find the start
                %
                while(~t_bReadMeasBlockStart)
                    tag = mne_rt_data_client.read_tag(t_inStream);

                    if(tag.kind == FIFF.FIFF_BLOCK_START && tag.data == FIFF.FIFFB_MEAS_INFO)
                        disp('FIFF_BLOCK_START FIFFB_MEAS_INFO'); 
                        t_bReadMeasBlockStart = true;
                    end
                end

                %
                % Parse until the endblock
                %
                
                info.dev_head_t = [];
                info.ctf_head_t = [];
                info.dev_ctf_t = [];
                info.acq_pars = [];
                info.acq_stim = [];
                info.chs = [];
  
                dev_head_t_read = false;
                ctf_head_t_read = false;

                while(~t_bReadMeasBlockEnd)
                    tag = mne_rt_data_client.read_tag(t_inStream);
                    %
                    %  megacq parameters
                    %
                    if(tag.kind == FIFF.FIFF_BLOCK_START && tag.data == FIFF.FIFFB_DACQ_PARS)                        
                        while(tag.kind ~= FIFF.FIFF_BLOCK_END || tag.data ~= FIFF.FIFFB_DACQ_PARS)
                            tag = mne_rt_data_client.read_tag(t_inStream);
                            if(tag.kind == FIFF.FIFF_DACQ_PARS)
                                info.acq_pars = tag.data;
                            elseif(tag.kind == FIFF.FIFF_DACQ_STIM)
                                info.acq_stim = tag.data;
                            end
                        end
                    end
                    %
                    %    Coordinate transformations if the HPI result block was not there
                    %
                    if (tag.kind == FIFF.FIFF_COORD_TRANS)
                        if (~dev_head_t_read)
                            info.dev_head_t = tag.data;
                            dev_head_t_read = true;
                        elseif (~ctf_head_t_read)
                            info.ctf_head_t = tag.data;
                            ctf_head_t_read = true;
                        end
                    end
                    %
                    %    Polhemus data
                    %
                    if(tag.kind == FIFF.FIFF_BLOCK_START && tag.data == FIFF.FIFFB_ISOTRAK)
                       info.dig = [];
                       while(tag.kind ~= FIFF.FIFF_BLOCK_END || tag.data ~= FIFF.FIFFB_ISOTRAK)
                            tag = mne_rt_data_client.read_tag(t_inStream);
                            if(tag.kind == FIFF.FIFF_DIG_POINT)
                                info.dig = [info.dig tag.data];
                            end
                        end
                    end
                    %
                    %    Projectors
                    %
                    if(tag.kind == FIFF.FIFF_BLOCK_START && tag.data == FIFF.FIFFB_PROJ)
                       info.projs = [];
                       while(tag.kind ~= FIFF.FIFF_BLOCK_END || tag.data ~= FIFF.FIFFB_PROJ)
                            tag = mne_rt_data_client.read_tag(t_inStream);
                            if(tag.kind == FIFF.FIFF_BLOCK_START && tag.data == FIFF.FIFFB_PROJ_ITEM)
                               proj = [];
                               while(tag.kind ~= FIFF.FIFF_BLOCK_END || tag.data ~= FIFF.FIFFB_PROJ_ITEM)
                                    tag = mne_rt_data_client.read_tag(t_inStream);
                                    switch tag.kind
                                        case FIFF.FIFF_NAME
                                            proj.desc = tag.data;
                                        case FIFF.FIFF_PROJ_ITEM_KIND
                                            proj.kind = tag.data;
                                        case FIFF.FIFF_NCHAN
                                            proj.data.ncol = tag.data;
                                        case FIFF.FIFF_PROJ_ITEM_NVEC
                                            proj.data.nrow = tag.data;
                                        case FIFF.FIFF_MNE_PROJ_ITEM_ACTIVE
                                            proj.active = tag.data;
                                        case FIFF.FIFF_PROJ_ITEM_CH_NAME_LIST
                                            proj.data.col_names = fiff_split_name_list(tag.data);
                                        case FIFF.FIFF_PROJ_ITEM_VECTORS
                                            proj.data.data = tag.data;
                                    end
                                end
                            end
                            
                            if ~isempty(proj)
                                info.projs = [info.projs proj];
                            end
                        end
                    end
                    %
                    %    CTF compensation info
                    %
                    if(tag.kind == FIFF.FIFF_BLOCK_START && tag.data == FIFF.FIFFB_MNE_CTF_COMP)
                       info.comps = [];
                       while(tag.kind ~= FIFF.FIFF_BLOCK_END || tag.data ~= FIFF.FIFFB_MNE_CTF_COMP)
                            tag = mne_rt_data_client.read_tag(t_inStream);
                            if(tag.kind == FIFF.FIFF_BLOCK_START && tag.data == FIFF.FIFFB_MNE_CTF_COMP_DATA)
                               comp = [];
                               while(tag.kind ~= FIFF.FIFF_BLOCK_END || tag.data ~= FIFF.FIFFB_MNE_CTF_COMP_DATA)
                                    tag = mne_rt_data_client.read_tag(t_inStream);
                                    switch tag.kind
                                        case FIFF.FIFF_MNE_CTF_COMP_KIND
                                            comp.ctfkind = tag.data;
                                        case FIFF.FIFF_MNE_CTF_COMP_CALIBRATED
                                            comp.save_calibrated = tag.data;
                                        case FIFF.FIFF_MNE_CTF_COMP_DATA
                                            comp.data = tag.data;
                                    end
                                end
                            end
                            
                            if ~isempty(comp)
                                info.comps = [info.comps comp];
                            end
                        end
                    end
                    %
                    %    Bad channels
                    %
                    if(tag.kind == FIFF.FIFF_BLOCK_START && tag.data == FIFF.FIFFB_MNE_BAD_CHANNELS)
                       info.bads = [];
                       while(tag.kind ~= FIFF.FIFF_BLOCK_END || tag.data ~= FIFF.FIFFB_MNE_BAD_CHANNELS)
                            tag = mne_rt_data_client.read_tag(t_inStream);
                            if(tag.kind == FIFF.FIFF_MNE_CH_NAME_LIST)
                                info.bads = fiff_split_name_list(tag.data);
                            end
                        end
                    end
                    %
                    %    General
                    %
                    if (tag.kind == FIFF.FIFF_SFREQ)
                        info.sfreq = tag.data;
                    elseif (tag.kind == FIFF.FIFF_HIGHPASS)
                        info.highpass = tag.data;
                    elseif (tag.kind == FIFF.FIFF_LOWPASS)
                        info.lowpass = tag.data;
                    elseif (tag.kind == FIFF.FIFF_NCHAN)
                        info.nchan = tag.data;
                    elseif (tag.kind == FIFF.FIFF_MEAS_DATE)
                        info.highpass = tag.data;
                    end
                        
                    
                    if (tag.kind == FIFF.FIFF_CH_INFO)
                        info.chs = [info.chs tag.data];
                    end
                    
                    % END MEAS
                    if(tag.kind == FIFF.FIFF_BLOCK_END && tag.data == FIFF.FIFFB_MEAS_INFO)
                        disp('FIFF_BLOCK_END FIFFB_MEAS_INFO'); 
                        t_bReadMeasBlockEnd = true;
                    end
                end
            else
                error('mne_rt_data_client: no available TcpSocket, call init to establish a connection.');
            end
        end
        
        % =================================================================
        %% readRawBuffer
        function [buf] = readRawBuffer(obj, p_nChannels)
            import java.net.Socket
            import java.io.*
              
            global FIFF;
            if isempty(FIFF)
                FIFF = fiff_define_constants();
            end
            
            buf = [];
            
            if ~isempty(obj.m_TcpSocket)
                % get a buffered data input stream from the socket
                t_inStream   = obj.m_TcpSocket.getInputStream;

                bytes_available = t_inStream.available;
                % Wait for incomming bytes
                while(bytes_available == 0)
                    bytes_available = p_inStream.available;
                end
                
                
                tag = mne_rt_data_client.read_tag(t_inStream);
                
                if(tag.kind == FIFF.FIFF_DATA_BUFFER)
                    nSamples = length(tag.data)/p_nChannels;
                    buf = reshape(tag.data, p_nChannels, nSamples);
                else
                    fprintf('tag is not of kind FIFF_DATA_BUFFER');
                end
            else
                error('mne_rt_data_client: no available TcpSocket, call init to establish a connection.');
            end
        end
        
        % =================================================================
        %% setClientAlias
        function [info] = setClientAlias(obj, alias)            
            import java.net.Socket
            import java.io.*

            global MNE_RT;
            if isempty(MNE_RT)
                MNE_RT = mne_rt_define_commands();
            end
            
            info = [];
            
            if ~isempty(obj.m_TcpSocket)
                % get a buffered data input stream from the socket
                t_outStream   = obj.m_TcpSocket.getOutputStream;

                mne_rt_data_client.sendFiffCommand(t_outStream, MNE_RT.MNE_RT_SET_CLIENT_ALIAS, alias)
            end
        end
        
        % =================================================================
        %% getClientId
        function [id] = getClientId(obj)
            import java.net.Socket
            import java.io.*
            if(obj.m_clientID == -1)
                global FIFF;
                if isempty(FIFF)
                    FIFF = fiff_define_constants();
                end
                global MNE_RT;
                if isempty(MNE_RT)
                    MNE_RT = mne_rt_define_commands();
                end

                if ~isempty(obj.m_TcpSocket)
                    % get a buffered data input stream from the socket
                    t_outStream   = obj.m_TcpSocket.getOutputStream;

                    mne_rt_data_client.sendFiffCommand(t_outStream, MNE_RT.MNE_RT_GET_CLIENT_ID)

                    % ID is send as answer
                    t_inStream   = obj.m_TcpSocket.getInputStream;
                    tag = obj.read_tag(t_inStream);
                    if (tag.kind == FIFF.FIFF_MNE_RT_CLIENT_ID)
                        obj.m_clientID = tag.data;
                    end                
                end
            end
            id = obj.m_clientID;
        end
    end
    
    methods(Static)
        % =================================================================
        %% sendFiffCommand
        function sendFiffCommand(p_outStream, p_Cmd, p_data)
            import java.net.Socket
            import java.io.*
            
            global FIFF;
            if isempty(FIFF)
                FIFF = fiff_define_constants();
            end
            
            if (nargin == 3)
                data = char(p_data);
            elseif(nargin == 2)
                data = [];
            else
                error('Wrong number of arguments.');
            end
            
            t_dOutputStream = DataOutputStream(p_outStream);
            
            kind = FIFF.FIFF_MNE_RT_COMMAND;
            type = FIFF.FIFFT_VOID;
            size = 4+length(data);% first 4 bytes are the command code
            next = 0;
            
            t_dOutputStream.writeInt(kind);
            t_dOutputStream.writeInt(type);
            t_dOutputStream.writeInt(size);
            t_dOutputStream.writeInt(next);
            t_dOutputStream.writeInt(p_Cmd);% first 4 bytes are the command code
            if(~isempty(data))
                t_dOutputStream.writeBytes(data);
            end
            t_dOutputStream.flush;
        end
        
        % =================================================================
        %% read_tag
        function [tag] = read_tag(p_inStream)
            import java.net.Socket
            import java.io.*
            
            me='MNE_RT_DATA_CLIENT:read_tag';
            
            %
            % read the tag info
            %
            tagInfo = mne_rt_data_client.read_tag_info(p_inStream);

            %
            % read the tag data
            %
            tag = mne_rt_data_client.read_tag_data(p_inStream, tagInfo);
        end
        
        % =================================================================
        %% read_tag_data
        function [tag] = read_tag_data(p_inStream, p_tagInfo, pos)
        %
        % [tag] = read_tag_data(p_dInputStream, pos)
        %
        % Read one tag from a fif stream.
        % if pos is not provided, reading starts from the current stream position
        %

        %
        %   Author : Christoph Dinh and Matti Hamalainen, MGH Martinos Center
        %   License : BSD 3-clause
        %
        
            import java.net.Socket
            import java.io.*
            
            global FIFF;
            if isempty(FIFF)
                FIFF = fiff_define_constants();
            end

            me='MNE_RT_DATA_CLIENT:read_tag_data';
            
%             t_bufferInputStream = BufferedInputStream(p_inStream);%to increase performance use a buffer in between
%             t_dInputStream = DataInputStream(t_bufferInputStream);

            t_dInputStream = DataInputStream(p_inStream);

            if nargin == 3
                t_dInputStream.skipBytes(pos);
            elseif nargin ~= 2
                error(me,'Incorrect number of arguments');
            end

            tag = p_tagInfo;

            %
            %   The magic hexadecimal values
            %
            is_matrix           = 4294901760; % ffff0000
            matrix_coding_dense = 16384;      % 4000
            matrix_coding_CCS   = 16400;      % 4010
            matrix_coding_RCS   = 16416;      % 4020
            data_type           = 65535;      % ffff
            %
            if tag.size > 0
                
%                 while true
%                     bytes_available = p_inStream.available;
% 
%                     if(bytes_available >= tagInfo.size)
%                         tag = mne_rt_data_client.read_tag_data(p_inStream, tagInfo);
%                         break;
%                     end
% 
%                     % pause 100ms before retrying
%                     pause(0.1);
%                 end
                
                
                
                matrix_coding = bitand(is_matrix,tag.type);
                if matrix_coding ~= 0
                    matrix_coding = bitshift(matrix_coding,-16);
                    %
                    %   Matrices
                    %
                    if matrix_coding == matrix_coding_dense
                        %
                        % Find dimensions and return to the beginning of tag data
                        %
                        
% Check can't be done in real-time --> moved to the end for reshape
%                         pos = ftell(fid);
%                         fseek(fid,tag.size-4,'cof');
%                         ndim = fread(fid,1,'int32');
%                         fseek(fid,-(ndim+1)*4,'cof');
%                         dims = fread(fid,ndim,'int32');
%                         %
%                         % Back to where the data start
%                         %
%                         fseek(fid,pos,'bof');

                        matrix_type = bitand(data_type,tag.type);

                        el_size = tag.size - 3*4; % 3*4 --> case 2D matrix; ToDo calculate el_size through

                        switch matrix_type
                            case FIFF.FIFFT_INT
                                tag.data = zeros(1, el_size/4);
                                for i = 1:el_size/4
                                    tag.data(i) = t_dInputStream.readInt;%idata = fread(fid,dims(1)*dims(2),'int32=>int32');
                                end
                            case FIFF.FIFFT_JULIAN
                                tag.data = zeros(1, el_size/4);
                                for i = 1:el_size/4
                                    tag.data(i) = t_dInputStream.readInt;%idata = fread(fid,dims(1)*dims(2),'int32=>int32');
                                end
                            case FIFF.FIFFT_FLOAT
                                tag.data = zeros(1, el_size/4);
                                for i = 1:el_size/4
                                    tag.data(i) = t_dInputStream.readFloat;%fdata = fread(fid,dims(1)*dims(2),'single=>double');
                                end
                           case FIFF.FIFFT_DOUBLE
                                tag.data = zeros(1, el_size/8);
                                for i = 1:el_size/8
                                    tag.data(i) = t_dInputStream.readDouble;%ddata = fread(fid,dims(1)*dims(2),'double=>double');
                                end
                            case FIFF.FIFFT_COMPLEX_FLOAT
                                tag.data = zeros(1, el_size/4);
                                for i = 1:el_size/4
                                    tag.data(i) = t_dInputStream.readFloat;%fdata = fread(fid,2*dims(1)*dims(2),'single=>double');
                                end
                                nel = length(tag.data);
                                tag.data = complex(tag.data(1:2:nel),tag.data(2:2:nel));
                                %
                                %   Note: we need the non-conjugate transpose here
                                %
%                                tag.data = transpose(reshape(fdata,dims(1),dims(2)));
                            case FIFF.FIFFT_COMPLEX_DOUBLE
                                tag.data = zeros(1, el_size/8);
                                for i = 1:el_size/8
                                    tag.data(i) = t_dInputStream.readDouble;%ddata = fread(fid,2*dims(1)*dims(2),'double=>double');
                                end
                                nel = length(tag.data);
                                tag.data = complex(tag.data(1:2:nel),tag.data(2:2:nel));
                                %
                                %   Note: we need the non-conjugate transpose here
                                %
%                                tag.data = transpose(reshape(ddata,dims(1),dims(2)));
                            otherwise
                                error(me,'Cannot handle a matrix of type %d yet',matrix_type)
                        end
                        
                        % ToDo consider 3D case --> do that by using tag->size
                        dims = zeros(1, 2);
                       
                        dims(1) = t_dInputStream.readInt;
                        dims(2) = t_dInputStream.readInt;
                        
                        ndim = t_dInputStream.readInt;
                        
                        tag.data = reshape(tag.data,dims(1),dims(2))';
                                                
                    if (matrix_type == FIFF.FIFFT_COMPLEX_FLOAT || matrix_type == FIFF.FIFFT_COMPLEX_DOUBLE)
                        %
                        %   Note: we need the non-conjugate transpose here
                        %
                        tag.data = transpose(tag.data);
                    end
%                elseif (matrix_coding == matrix_coding_CCS || matrix_coding == matrix_coding_RCS)
%                         %
%                         % Find dimensions and return to the beginning of tag data
%                         %
%                         pos = ftell(fid);
%                         fseek(fid,tag.size-4,'cof');
%                         ndim = fread(fid,1,'int32');
%                         fseek(fid,-(ndim+2)*4,'cof');
%                         dims = fread(fid,ndim+1,'int32');
%                         if ndim ~= 2
%                             error(me,'Only two-dimensional matrices are supported at this time');
%                         end
%                         %
%                         % Back to where the data start
%                         %
%                         fseek(fid,pos,'bof');
%                         nnz   = dims(1);
%                         nrow  = dims(2);
%                         ncol  = dims(3);
%                         sparse_data = zeros(nnz,3);
%                         sparse_data(:,3) = fread(fid,nnz,'single=>double');
%                         if (matrix_coding == matrix_coding_CCS)
%                             %
%                             %    CCS
%                             %
%                             sparse_data(:,1)  = fread(fid,nnz,'int32=>double') + 1;
%                             ptrs  = fread(fid,ncol+1,'int32=>double') + 1;
%                             p = 1;
%                             for j = 1:ncol
%                                 while p < ptrs(j+1)
%                                     sparse_data(p,2) = j;
%                                     p = p + 1;
%                                 end
%                             end
%                         else
%                             %
%                             %    RCS
%                             %
%                             sparse_data(:,2)  = fread(fid,nnz,'int32=>double') + 1;
%                             ptrs  = fread(fid,nrow+1,'int32=>double') + 1;
%                             p = 1;
%                             for j = 1:nrow
%                                 while p < ptrs(j+1)
%                                     sparse_data(p,1) = j;
%                                     p = p + 1;
%                                 end
%                             end
%                         end
%                         tag.data = spconvert(sparse_data);
%                         tag.data(nrow,ncol) = 0.0;
                    else
                        error(me,'Cannot handle other than dense or sparse matrices yet')
                    end
                else
                    %
                    %   All other data types
                    %
                    switch tag.type
                        %
                        %   Simple types
                        %
                        case FIFF.FIFFT_BYTE
                            tag.data = zeros(1, tag.size);
                            for i = 1:tag.size
                                tag.data(i) = t_dInputStream.readUnsignedByte;%fread(fid,tag.size,'uint8=>uint8');
                            end
                        case FIFF.FIFFT_SHORT
                            tag.data = zeros(1, tag.size/2);
                            for i = 1:tag.size/2
                                tag.data(i) = t_dInputStream.readShort;%fread(fid,tag.size/2,'int16=>int16');
                            end
                        case FIFF.FIFFT_INT
                            tag.data = zeros(1, tag.size/4);
                            for i = 1:tag.size/4
                                tag.data(i) = t_dInputStream.readInt;%fread(fid,tag.size/4,'int32=>int32');
                            end
                        case FIFF.FIFFT_USHORT
                            tag.data = zeros(1, tag.size/2);
                            for i = 1:tag.size/2
                                tag.data(i) = t_dInputStream.readUnsignedShort;%fread(fid,tag.size/2,'uint16=>uint16');
                            end
                        case FIFF.FIFFT_UINT
                            tag.data = zeros(1, tag.size/4);
                            for i = 1:tag.size/4
                                tag.data(i) = t_dInputStream.readInt;%fread(fid,tag.size/4,'uint32=>uint32');
                            end
                        case FIFF.FIFFT_FLOAT
                            tag.data = zeros(1, tag.size/4);
                            for i = 1:tag.size/4
                                tag.data(i) = t_dInputStream.readFloat;%fread(fid,tag.size/4,'single=>double');
                            end
            %             case FIFF.FIFFT_DOUBLE
            %                 tag.data = fread(fid,tag.size/8,'double');
                        case FIFF.FIFFT_STRING
                            tag.data = zeros(1, tag.size);
                            for i = 1:tag.size
                                tag.data(i) = t_dInputStream.readByte;%fread(fid,tag.size,'uint8=>char')';
                            end
                            tag.data = char(tag.data);                            
            %             case FIFF.FIFFT_DAU_PACK16
            %                 tag.data = fread(fid,tag.size/2,'int16=>int16');
            %             case FIFF.FIFFT_COMPLEX_FLOAT
            %                 tag.data = fread(fid,tag.size/4,'single=>double');
            %                 nel = length(tag.data);
            %                 tag.data = complex(tag.data(1:2:nel),tag.data(2:2:nel));
            %             case FIFF.FIFFT_COMPLEX_DOUBLE
            %                 tag.data = fread(fid,tag.size/8,'double');
            %                 nel = length(tag.data);
            %                 tag.data = complex(tag.data(1:2:nel),tag.data(2:2:nel));
            %                 %
            %                 %   Structures
            %                 %
                        case FIFF.FIFFT_ID_STRUCT
                            tag.data.version = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.machid = zeros(2,1);
                            tag.data.machid(1)  = t_dInputStream.readInt;%fread(fid,2,'int32=>int32');
                            tag.data.machid(2)  = t_dInputStream.readInt;
                            tag.data.secs    = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.usecs   = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                        case FIFF.FIFFT_DIG_POINT_STRUCT
                            tag.data.kind    = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.ident   = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.r = zeros(3,1);
                            for i = 1:3
                                tag.data.r(i)	= t_dInputStream.readFloat;%fread(fid,3,'single=>single');
                            end
                            tag.data.coord_frame = 0;
                        case FIFF.FIFFT_COORD_TRANS_STRUCT
                            tag.data.from = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.to   = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            rot = zeros(9,1);
                            for i = 1:9
                                rot(i) = t_dInputStream.readFloat;%fread(fid,9,'single=>double');
                            end
                            rot = reshape(rot,3,3)';
                            move = zeros(3,1);
                            for i = 1:3
                                move(i) = t_dInputStream.readFloat;%fread(fid,3,'single=>double');
                            end
                            tag.data.trans = [ rot move ; [ 0  0 0 1 ]];
                            %
                            % Skip over the inverse transformation
                            % It is easier to just use inverse of trans in Matlab
                            %
                            for i = 1:12 %fseek(fid,12*4,'cof');
                                t_dInputStream.readFloat;
                            end
                        case FIFF.FIFFT_CH_INFO_STRUCT
                            tag.data.scanno    = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.logno     = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.kind      = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.range     = t_dInputStream.readFloat;%fread(fid,1,'single=>double');
                            tag.data.cal       = t_dInputStream.readFloat;%fread(fid,1,'single=>double');
                            tag.data.coil_type = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            %
                            %   Read the coil coordinate system definition
                            %
                            tag.data.loc = zeros(12,1);
                            for i = 1:12
                                tag.data.loc(i) = t_dInputStream.readFloat;%fread(fid,12,'single=>double');
                            end
                            tag.data.coil_trans  = [];
                            tag.data.eeg_loc     = [];
                            tag.data.coord_frame = FIFF.FIFFV_COORD_UNKNOWN;
                            %
                            %   Convert loc into a more useful format
                            %
                            loc = tag.data.loc;
                            if tag.data.kind == FIFF.FIFFV_MEG_CH || tag.data.kind == FIFF.FIFFV_REF_MEG_CH
                                tag.data.coil_trans  = [ [ loc(4:6) loc(7:9) loc(10:12) loc(1:3) ] ; [ 0 0 0 1 ] ];
                                tag.data.coord_frame = FIFF.FIFFV_COORD_DEVICE;
                            elseif tag.data.kind == FIFF.FIFFV_EEG_CH
                                if norm(loc(4:6)) > 0
                                    tag.data.eeg_loc     = [ loc(1:3) loc(4:6) ];
                                else
                                    tag.data.eeg_loc = [ loc(1:3) ];
                                end
                                tag.data.coord_frame = FIFF.FIFFV_COORD_HEAD;
                            end
                            %
                            %   Unit and exponent
                            %
                            tag.data.unit     = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            tag.data.unit_mul = t_dInputStream.readInt;%fread(fid,1,'int32=>int32');
                            %
                            %   Handle the channel name
                            %
                            ch_name = zeros(1, 16, 'uint8');
                            for i = 1:16
                                ch_name(i) = t_dInputStream.readByte;
                            end
                            ch_name   = char(ch_name);
                            %
                            % Omit nulls
                            %
                            len = 16;
                            for k = 1:16
                                if ch_name(k) == 0
                                    len = k-1;
                                    break
                                end
                            end
                            tag.data.ch_name = ch_name(1:len);
            %             case FIFF.FIFFT_OLD_PACK
            %                 offset   = fread(fid,1,'single=>double');
            %                 scale    = fread(fid,1,'single=>double');
            %                 tag.data = fread(fid,(tag.size-8)/2,'int16=>short');
            %                 tag.data = scale*single(tag.data) + offset;
            %             case FIFF.FIFFT_DIR_ENTRY_STRUCT
            %                 tag.data = struct('kind',{},'type',{},'size',{},'pos',{});
            %                 for k = 1:tag.size/16-1
            %                     kind = fread(fid,1,'int32');
            %                     type = fread(fid,1,'uint32');
            %                     tagsize = fread(fid,1,'int32');
            %                     pos  = fread(fid,1,'int32');
            %                     tag.data(k).kind = kind;
            %                     tag.data(k).type = type;
            %                     tag.data(k).size = tagsize;
            %                     tag.data(k).pos  = pos;
            %                 end
            %                 
                        otherwise
                            error(me,'Unimplemented tag data type %d',tag.type);

                    end
                end
            end

            % if tag.next ~= FIFF.FIFFV_NEXT_SEQ
            %     fseek(fid,tag.next,'bof');
            % end

            return;
        end
            
        % =================================================================
        %% read_tag_info
        function [tag] = read_tag_info(p_inStream, pos)
        %
        % [tag] = read_tag_info(p_inStream, pos)
        %
        % Read one tag from a fif stream.
        % if pos is not provided, reading starts from the current stream position
        %

        %
        %   Author : Christoph Dinh and Matti Hamalainen, MGH Martinos Center
        %   License : BSD 3-clause
        %
            import java.net.Socket
            import java.io.*
            
            global FIFF;
            if isempty(FIFF)
                FIFF = fiff_define_constants();
            end

            me='MNE_RT_DATA_CLIENT:read_tag_info';

            t_dInputStream = DataInputStream(p_inStream);
            
            if nargin == 2
                t_dInputStream.skipBytes(pos);
            elseif nargin ~= 1
                error(me,'Incorrect number of arguments');
            end
            
%             while true
%                 bytes_available = p_inStream.available;
%                 if(bytes_available >= 16)
            tag.kind = t_dInputStream.readInt;
            tag.type = t_dInputStream.readInt;
            tag.size = t_dInputStream.readInt;
            tag.next = t_dInputStream.readInt;
%                     break;
%                 end
%                 % pause 100ms before retrying
%                 pause(0.1);
%             end
            
            return;
        end                    
    end
end

