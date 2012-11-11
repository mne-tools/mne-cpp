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
        %% sendCommand
        function [info] = sendCommand(obj,p_sCommand)
            import java.net.Socket
            import java.io.*
            
            t_sCommand = sprintf('%s\n',p_sCommand);
            info = [];
            
            if ~isempty(obj.m_DataOutputStream) && ~isempty(obj.m_DataInputStream)
                
                obj.m_DataOutputStream.writeBytes(t_sCommand);
                obj.m_DataOutputStream.flush;

                % read data from the socket - wait a short time first
                pause(0.5);
                bytes_available = obj.m_DataInputStream.available;

                info = zeros(1, bytes_available, 'uint8');
                for i = 1:bytes_available
                    info(i) = obj.m_DataInputStream.readByte;
                end

                info = char(info);              
            end
        end
        
        % =================================================================
        %% requestMeasInfo
        function requestMeasInfo(obj, AliasOrId)
            import java.net.Socket
            import java.io.*
            
            if(ischar(AliasOrId))
                command = sprintf('measinfo %s\n', AliasOrId);
            elseif(isnumeric(AliasOrId))
                command = sprintf('measinfo %d\n', AliasOrId);
            else
                error('unknown format for AliasOrId');
            end
            
            obj.sendCommand(command);
        end
        
        % =================================================================
        %% requestMeas
        function requestMeas(obj, AliasOrId)
            import java.net.Socket
            import java.io.*
            
            if(ischar(AliasOrId))
                command = sprintf('meas %s\n', AliasOrId);
            elseif(isnumeric(AliasOrId))
                command = sprintf('meas %d\n', AliasOrId);
            else
                error('unknown format for AliasOrId');
            end
            
            obj.sendCommand(command);
        end
    end
end

