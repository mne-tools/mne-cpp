classdef mne_rt_client < handle
    %MNE_RT_CLIENT Summary of this class goes here
    %   Detailed explanation goes here
    
    properties (Access = public)
        m_DataInputStream = [];
        m_DataOutputStream = [];
    end
    
    properties (Access = private)
        m_TcpSocket = [];
        m_InputStream = [];
        m_OutputStream = [];
        m_BufferedInputStream = []; %to increase performance use a buffer in between
        m_iNumOfRetries = -1;
    end
    
    methods
        % =================================================================
        %% mne_rt_client
        function obj = mne_rt_client(host, port, number_of_retries)
            obj.init(host, port, number_of_retries);
        end % mne_rt_client
        
        % =================================================================
        %% init
        function result = init(obj, host, port, numOfRetries) 

            import java.net.Socket
            import java.io.*

            if (nargin < 3)
                obj.m_iNumOfRetries = 20; % set to -1 for infinite
            end

            retry = 0;
            obj.close();
            
            result      = false;

            while true

                retry = retry + 1;
                if ((obj.m_iNumOfRetries > 0) && (retry > obj.m_iNumOfRetries))
                    fprintf(1, 'Too many retries\n');
                    break;
                end

                try
                    fprintf(1, 'Retry %d connecting to %s:%d\n', ...
                            retry, host, port);

                    % throws if unable to connect
                    obj.m_TcpSocket = Socket(host, port);

                    obj.m_InputStream = obj.m_TcpSocket.getInputStream;
                    obj.m_BufferedInputStream = BufferedInputStream(obj.m_InputStream);
                    obj.m_DataInputStream = DataInputStream(obj.m_BufferedInputStream);

                    obj.m_OutputStream = obj.m_TcpSocket.getOutputStream;
                    obj.m_DataOutputStream = DataOutputStream(obj.m_OutputStream);
                    
                    

                    fprintf(1, 'Connected to server\n');
                    
                    result = true;
                    
                    break;

                catch
                    obj.close();

                    % pause before retrying
                    pause(1);
                end
            end
        end

        function close(obj)
            if ~isempty(obj.m_TcpSocket)
                obj.m_TcpSocket.close();
            end
            obj.m_TcpSocket = [];
            obj.m_InputStream = [];
            obj.m_OutputStream = [];
            obj.m_BufferedInputStream = []; 
            obj.m_DataInputStream = [];
            obj.m_DataOutputStream = [];
        end
    end
end


