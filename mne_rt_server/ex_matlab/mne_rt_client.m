classdef mne_rt_client < handle
    %MNE_RT_CLIENT Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        m_TcpSocket = [];
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

            retry        = 0;
            obj.m_TcpSocket = [];
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
        end
    end
end


