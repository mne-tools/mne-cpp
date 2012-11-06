classdef mne_rt_cmd_client < mne_rt_client
    %MNE_RT_CMD_CLIENT Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
    end
    
    methods
        % =================================================================
        %% mne_rt_cmd_client
        function obj = mne_rt_cmd_client(host, port, numOfRetries)
            if (nargin < 3)
                numOfRetries = 20; % set to -1 for infinite
            end
            obj = obj@mne_rt_client(host, port, numOfRetries);%Superclass call
        end % mne_rt_cmd_client
        
        % =================================================================
        %% getClientList
        function [info] = getClientList(obj)
            import java.net.Socket
            import java.io.*
            
            command = sprintf('clist\n');
            info = [];
            
            if ~isempty(obj.m_TcpSocket)
                % get a buffered data output stream from the socket
                t_outStream   = obj.m_TcpSocket.getOutputStream;
                t_dataOutStream = DataOutputStream(t_outStream);
                
                t_dataOutStream.writeBytes(command);
                t_dataOutStream.flush;
                
                
                % get a buffered data input stream from the socket
                t_inStream   = obj.m_TcpSocket.getInputStream;
                t_dataInStream = DataInputStream(t_inStream);
                % read data from the socket - wait a short time first
                pause(0.5);
                bytes_available = t_dataInStream.available;

                info = zeros(1, bytes_available, 'uint8');
                for i = 1:bytes_available
                    info(i) = t_dataInStream.readByte;
                end

                info = char(info);              
            end
        end
    end
end

