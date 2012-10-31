classdef mne_rt_client < handle
    %MNE_MATLAB_CLIENT Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        m_input_socket = [];
        number_of_retries = -1;
    end
    
    methods
        %% mne_matlab_client
        function obj = mne_rt_client(varargin)
            
        end % mne_matlab_client
        
        %% init_connection
        function result = init_connection(obj, host, port, number_of_retries) 

            import java.net.Socket
            import java.io.*

            if (nargin < 3)
                obj.number_of_retries = 20; % set to -1 for infinite
            end

            retry        = 0;
            obj.m_input_socket = [];
            result      = false;

            while true

                retry = retry + 1;
                if ((obj.number_of_retries > 0) && (retry > obj.number_of_retries))
                    fprintf(1, 'Too many retries\n');
                    break;
                end

                try
                    fprintf(1, 'Retry %d connecting to %s:%d\n', ...
                            retry, host, port);

                    % throws if unable to connect
                    obj.m_input_socket = Socket(host, port);

                    fprintf(1, 'Connected to server\n');
                    
                    result = true;
                    
                    break;

                catch
                    if ~isempty(obj.m_input_socket)
                        obj.m_input_socket.close;
                    end

                    % pause before retrying
                    pause(1);
                end
            end
        end

        %% read_tag_stream
        function [tag] = read_tag_stream(obj)
            
            import java.net.Socket
            import java.io.*

            global FIFF;
            if isempty(FIFF)
                FIFF = fiff_define_constants();
            end

            global MNE_RT;
            if isempty(MNE_RT)
                MNE_RT = mne_rt_define_constants();
            end
            
            me='MNE_RT_CLIENT:read_tag_stream';
            
            tag = [];

            if ~isempty(obj.m_input_socket)
                % get a buffered data input stream from the socket
                input_stream   = obj.m_input_socket.getInputStream;
                d_input_stream = DataInputStream(input_stream);

                % read data from the socket
                bytes_available = input_stream.available;
                fprintf(1, 'Reading %d bytes\n', bytes_available);
                
                tag = fiff_read_tag_stream(d_input_stream);
                
                
                if(tag.kind == FIFF.FIFF_MNE_RT_COMMAND)
                    switch tag.data(1)
                    %
                    %   MNE_RT constants are stored in the first data
                    %
                    case MNE_RT.MNE_RT_CLIENT_ID
                        fprintf(1, 'MNE_RT_CLIENT_ID\n');
                    otherwise
                        error(me,'Unimplemented tag kind %d',tag.kind); 
                    end
                end
                
                
                bytes_available = input_stream.available;
                if(bytes_available > 0)
                    printf(1, '%d bytes more to read\n', bytes_available);
                end
            end
        end
    end
end

