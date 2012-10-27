classdef mne_matlab_client < handle
    %MNE_MATLAB_CLIENT Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        m_input_socket = [];
        number_of_retries = -1;
    end
    
    methods
        %% mne_matlab_client
        function obj = mne_matlab_client(varargin)
            
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

        %% read
        function [blocksize, raw] = read(obj)

            import java.net.Socket
            import java.io.*
            
            blocksize = -1;
            raw = [];

            if ~isempty(obj.m_input_socket)
                % get a buffered data input stream from the socket
                input_stream   = obj.m_input_socket.getInputStream;
                d_input_stream = DataInputStream(input_stream);

                % read data from the socket
                bytes_available = input_stream.available;
                fprintf(1, 'Reading %d bytes\n', bytes_available);

                blocksize = d_input_stream.readUnsignedShort;
                
                bytes_available = input_stream.available;
                data = zeros(1, bytes_available, 'uint8');
                for i = 1:bytes_available
                    data(i) = d_input_stream.readByte;
                end
                
                raw = char(data(5:end));
            end
        end
    end
end

