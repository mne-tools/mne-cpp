# Default Connector
* This default connector can be used as a template for other connectors.
* Furthermore this connector implements some basic commands.

# Guidelines
* A connector must implement the following interfaces:
 * mne_rt_server/IConnectornew.h - All necessary method for the server to interact with the connector. (Will be renamed to IConnector)
 * mne_rt_server/ICommandRequestManager.h - Interprets the commands and sets the logic/actions.
 * rt_communication/ICommandRequest.h - All commands which should be provided.
 * rt_communication/ICommandResponse.h - Optional: For an API-like access of the responses.
 * ICommandRequest & ICommandResponse must not implement the logic which should be triggered!
* Folder structure:
 * necessary for an easy external include
 * ./command_requests - All implementations of ICommandRequest
 * ./command_responses - All implementations of ICommandResponse
