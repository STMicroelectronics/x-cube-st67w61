\mainpage Release Notes for ST67W6X Network Driver Middleware

============
\tableofcontents

# Release Notes for ST67W6X Network Driver Middleware

Copyright &copy; 2025-2026 STMicroelectronics

![](st_logo_2020.png)

## Purpose

This Middleware is controlling the **ST67W611M** Low-power Wi-Fi 6 & Bluetooth LE combo coprocessor module from STMicroelectronics. It exposes an API corresponding to the Network interface on Wi-Fi or BLE protocols.

The ST67W6X Network Driver is responsible for:

- Providing API to manage Wi-Fi, BLE, Network sockets, Network direct link, MQTT, HTTP, Firmware updates or System features.
- Transporting messages between the host and the ST67W611M over the underlying bus via AT command over SPI Interface.

It is comprised of the following 8 folders :

- **Api** : Exposed APIs available for the application.
- **Conf** : This middleware configuration template files.
- **Core** : Implementation of API and common components to different drivers and few higher layer functionality (check states, blocking calls, etc).
- **Driver/W61_at** : ST67W611M AT commands implementation with parser and formatter processing.
- **Driver/W61_bus** : ST67W611M communication interface over SPI bus.
- **Utils/Logging** : Logging utility to manage the print traces over UART or ITM.
- **Utils/Misc** : Miscellaneous utility functions.
- **Utils/Performance** : Iperf, memory and task performance utilities.
- **Utils/Shell** : Shell utility to manage the command line interface.

The scope of the Utils:

- **Utils/Logging** : Called by the W61_at driver, the service APIs and can be also called by the application.
- **Utils/Misc** : Called by the W61_at driver, the service APIs and can be also called by the application.
- **Utils/Performance** : Called by the application as predefined examples code. Some are based on the service APIs.
- **Utils/Shell** : Called by the service APIs and can be also called by the application.

This Middleware and drivers require FreeRTOS.

## API introduction

ST67W6X Network Driver Middleware provides a set of APIs to manage the Wi-Fi, BLE, Sockets, MQTT or System features.

### System API Functions

The following functions are available in the \ref ST67W6X_API_System_Public_Functions "System API group":

| Function Name               | Description                                                                                                                              |
| --------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `W6X_Init`                  | Initialize the LL part of the W6X core (call once at startup before any other W6X_* API; then register callbacks with W6X_RegisterAppCb) |
| `W6X_DeInit`                | De-Initialize the LL part of the W6X core.                                                                                               |
| `W6X_RegisterAppCb`         | Register Upper Layer callbacks (used for asynchronous events; callback table must remain valid while driver is initialized)              |
| `W6X_GetCbHandler`          | Get the W6X Callback handler.                                                                                                            |
| `W6X_GetModuleInfo`         | Get the W6X module info.                                                                                                                 |
| `W6X_ModuleInfoDisplay`     | Display the module information.                                                                                                          |
| `W6X_SetPowerMode`          | Configure Power mode (sets the NCP power-save mode)                                                                                      |
| `W6X_GetPowerMode`          | Get Power mode.                                                                                                                          |
| `W6X_FS_WriteFileByName`    | Write a file from the Host file system to the NCP file system (sends the host file identified by filename to the NCP filesystem)         |
| `W6X_FS_WriteFileByContent` | Write a file from the local memory to the NCP file system (writes a new NCP file using a RAM buffer as source)                           |
| `W6X_FS_ReadFile`           | Read a file from the NCP file system (reads len bytes starting at the given offset)                                                      |
| `W6X_FS_DeleteFile`         | Delete a file from the NCP file system (deletes a file from the NCP filesystem)                                                          |
| `W6X_FS_GetSizeFile`        | Get the size of a file in the NCP file system.                                                                                           |
| `W6X_FS_ListFiles`          | List files in the file system (NCP and Host if LFS is enabled) (maximum number of files is W6X_SYS_FS_MAX_FILES)                         |
| `W6X_Reset`                 | Reset module.                                                                                                                            |
| `W6X_ExeATCommand`          | Execute AT command (sends a raw AT command string to the NCP)                                                                            |
| `W6X_StatusToStr`           | Convert the W6X status to a string.                                                                                                      |
| `W6X_ModelToStr`            | Convert the W6X module ID to a string.                                                                                                   |
| `W6X_SdkMinVersion`         | Check that the SDK version is at least the specified version.                                                                            |

### FWU API Functions

The following functions are available in the \ref ST67W6X_API_FWU_Public_Functions "FWU API group":

| Function Name    | Description                                                                                                                                |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| `W6X_FWU_Starts` | Starts the NCP FWU process (start/stop the NCP firmware update transfer session)                                                           |
| `W6X_FWU_Finish` | Finish the NCP FWU process which reboot the module to apply the new firmware.                                                              |
| `W6X_FWU_Send`   | Send the firmware binary to the module (sends a chunk of the firmware image; use in a loop between ::W6X_FWU_Starts(1) and W6X_FWU_Finish) |

### WiFi API Functions

The following functions are available in the \ref ST67W6X_API_WiFi_Public_Functions "WiFi API group":

| Function Name                       | Description                                                                                                                                                 |
| ----------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `W6X_WiFi_Init`                     | Init the Wi-Fi module (initializes the Wi-Fi subsystem; call after W6X_Init)                                                                                |
| `W6X_WiFi_DeInit`                   | De-Init the WiFi module.                                                                                                                                    |
| `W6X_WiFi_Scan`                     | List a defined number of available access points (starts a scan and reports results through the provided callback)                                          |
| `W6X_WiFi_PrintScan`                | Print the scan results.                                                                                                                                     |
| `W6X_WiFi_Connect`                  | Join an Access Point (connects the station interface using the provided options)                                                                            |
| `W6X_WiFi_Disconnect`               | Disconnect from a Wi-Fi Network (disconnects the station interface from the current AP)                                                                     |
| `W6X_WiFi_AddCredentials`           | Add Wi-Fi station credentials in the NCP (stores an SSID/password pair in the NCP credential store; can later be used by W6X_WiFi_Connect and auto-connect) |
| `W6X_WiFi_DeleteCredentials`        | Delete Wi-Fi station credentials from the NCP (removes a previously stored SSID from the NCP credential store)                                              |
| `W6X_WiFi_GetCredentials`           | List the stored Wi-Fi station credentials in the NCP (contains up to W6X_WIFI_MAX_SSID_LIST_SIZE entries)                                                   |
| `W6X_WiFi_GetAutoConnect`           | Retrieve auto connect state.                                                                                                                                |
| `W6X_WiFi_GetCountryCode`           | This function retrieves the country code configuration.                                                                                                     |
| `W6X_WiFi_SetCountryCode`           | This function set the country code configuration.                                                                                                           |
| `W6X_WiFi_Station_Start`            | Set the module in station mode.                                                                                                                             |
| `W6X_WiFi_Station_GetState`         | This function retrieves the Wi-Fi station state (can be used to poll station status and retrieve the latest connection data)                                |
| `W6X_WiFi_Station_GetMACAddress`    | This function retrieves the Wi-Fi station MAC address.                                                                                                      |
| `W6X_WiFi_AP_Start`                 | Configure a Soft-AP.                                                                                                                                        |
| `W6X_WiFi_AP_Stop`                  | Stop the Soft-AP.                                                                                                                                           |
| `W6X_WiFi_AP_GetConfig`             | Get the Soft-AP configuration.                                                                                                                              |
| `W6X_WiFi_AP_ListConnectedStations` | List the connected stations (maximum number of connected stations is W6X_WIFI_MAX_CONNECTED_STATIONS)                                                       |
| `W6X_WiFi_AP_DisconnectStation`     | Disconnect station from the Soft-AP.                                                                                                                        |
| `W6X_WiFi_AP_GetMACAddress`         | This function retrieves the Wi-Fi Soft-AP MAC address.                                                                                                      |
| `W6X_WiFi_SetDTIM`                  | Set Low Power Wi-Fi DTIM (Delivery Traffic Indication Message)                                                                                              |
| `W6X_WiFi_GetDTIM`                  | Get Low Power Wi-Fi DTIM (Delivery Traffic Indication Message) (dtim_interval is derived from the configured factor and the AP DTIM)                        |
| `W6X_WiFi_GetDTIM_AP`               | Get Low Power Wi-Fi DTIM (Delivery Traffic Indication Message) for the Access Point.                                                                        |
| `W6X_WiFi_TWT_Setup`                | Setup Target Wake Time (TWT) for the Wi-Fi station (supports up to W6X_WIFI_MAX_TWT_FLOWS simultaneous TWT flows)                                           |
| `W6X_WiFi_TWT_GetStatus`            | Get Target Wake Time (TWT) status for the Wi-Fi station.                                                                                                    |
| `W6X_WiFi_TWT_Teardown`             | Teardown Target Wake Time (TWT) for the Wi-Fi station.                                                                                                      |
| `W6X_WiFi_GetAntennaDiversity`      | Get the antenna diversity information.                                                                                                                      |
| `W6X_WiFi_SetAntennaDiversity`      | Set the antenna diversity configuration.                                                                                                                    |
| `W6X_WiFi_StateToStr`               | Convert the Wi-Fi state to a string.                                                                                                                        |
| `W6X_WiFi_SecurityToStr`            | Convert the Wi-Fi security type to a string.                                                                                                                |
| `W6X_WiFi_AP_SecurityToStr`         | Convert the Wi-Fi AP security type to a string.                                                                                                             |
| `W6X_WiFi_ReasonToStr`              | Convert the Wi-Fi error reason to a string.                                                                                                                 |
| `W6X_WiFi_ProtocolToStr`            | Convert the Wi-Fi protocol to a string.                                                                                                                     |
| `W6X_WiFi_AntDivToStr`              | Convert the Wi-Fi antenna mode to a string.                                                                                                                 |

### Net API Functions

The following functions are available in the \ref ST67W6X_API_Net_Public_Functions "Net API group":

| Function Name                         | Description                                                                                                |
| ------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| `W6X_Net_Init`                        | Init the Net module.                                                                                       |
| `W6X_Net_DeInit`                      | De-Init the Net module.                                                                                    |
| `W6X_Net_SetHostname`                 | Set the Wi-Fi Station host name.                                                                           |
| `W6X_Net_GetHostname`                 | Get the Wi-Fi Station host name.                                                                           |
| `W6X_Net_Station_GetIPAddress`        | Get the Wi-Fi Station interface's IP address.                                                              |
| `W6X_Net_Station_GetIPv6Address`      | Get the Wi-Fi Station interface's IPv6 addresses (link-local & global)                                     |
| `W6X_Net_Station_SetIPAddress`        | Set the Wi-Fi Station interface's IP address.                                                              |
| `W6X_Net_AP_GetIPAddress`             | Get the Soft-AP IP addresses.                                                                              |
| `W6X_Net_AP_SetIPAddress`             | Set the Soft-AP IP addresses.                                                                              |
| `W6X_Net_GetDhcp`                     | Get the DHCP configuration.                                                                                |
| `W6X_Net_SetDhcp`                     | Set the DHCP configuration.                                                                                |
| `W6X_Net_GetDnsAddress`               | Get the Wi-Fi DNS addresses.                                                                               |
| `W6X_Net_SetDnsAddress`               | Set the Wi-Fi DNS addresses.                                                                               |
| `W6X_Net_Ping`                        | Ping an IP address in the network (issues ICMP echo requests and returns aggregate results)                |
| `W6X_Net_ResolveHostAddress`          | Get IP address from URL using DNS.                                                                         |
| `W6X_Net_ResolveHostAddressByType`    | Resolve a hostname to an IP (IPv4 or IPv6).                                                                |
| `W6X_Net_SNTP_GetConfiguration`       | Get current SNTP status, timezone and servers (retrieves current SNTP configuration)                       |
| `W6X_Net_SNTP_SetConfiguration`       | Set SNTP status, timezone and servers (the NCP uses the configured servers to synchronize its system time) |
| `W6X_Net_SNTP_GetInterval`            | Get SNTP Synchronization interval.                                                                         |
| `W6X_Net_SNTP_SetInterval`            | Set SNTP Synchronization interval.                                                                         |
| `W6X_Net_SNTP_GetTime`                | Query date string from SNTP, the used format is asctime style time.                                        |
| `W6X_Net_GetConnectionStatus`         | Get information for an opened socket.                                                                      |
| `W6X_Net_Socket`                      | Get a socket instance is available.                                                                        |
| `W6X_Net_Close`                       | Close a socket instance and release the associated resources.                                              |
| `W6X_Net_Shutdown`                    | Shutdown a socket instance.                                                                                |
| `W6X_Net_Bind`                        | Bind a socket instance to a specific address.                                                              |
| `W6X_Net_Connect`                     | Connect a socket instance to a specific address.                                                           |
| `W6X_Net_Listen`                      | Listen for incoming connections on a socket.                                                               |
| `W6X_Net_Accept`                      | Accept an incoming connection on a socket.                                                                 |
| `W6X_Net_Send`                        | Send data on a socket.                                                                                     |
| `W6X_Net_Recv`                        | Receive data from a socket.                                                                                |
| `W6X_Net_Sendto`                      | Send data on a socket to a specific address.                                                               |
| `W6X_Net_Recvfrom`                    | Receive data from a socket from a specific address.                                                        |
| `W6X_Net_Getsockopt`                  | Get a socket option.                                                                                       |
| `W6X_Net_Setsockopt`                  | Set a socket option.                                                                                       |
| `W6X_Net_TLS_Credential_AddByContent` | Add the credential by local file content to the TLS context.                                               |
| `W6X_Net_TLS_Credential_AddByName`    | Add the credential by name from Host LFS to the TLS context.                                               |
| `W6X_Net_TLS_Credential_Delete`       | Delete the credential from the TLS context.                                                                |
| `W6X_Net_Inet_ntop`                   | Convert an IP address from uint32_t format to text.                                                        |
| `W6X_Net_Inet_pton`                   | Convert an IP address from text format to uint32_t.                                                        |
| `W6X_Net_Inet6_aton`                  | Convert an IPv6 text representation to binary (struct in6_addr) via driver aton.                           |

### HTTP API Functions

The following functions are available in the \ref ST67W6X_API_HTTP_Public_Functions "HTTP API group":

| Function Name             | Description                              |
| ------------------------- | ---------------------------------------- |
| `W6X_HTTP_Client_Request` | HTTP Client request based on BSD socket. |

### MQTT API Functions

The following functions are available in the \ref ST67W6X_API_MQTT_Public_Functions "MQTT API group":

| Function Name                  | Description                                                                                              |
| ------------------------------ | -------------------------------------------------------------------------------------------------------- |
| `W6X_MQTT_Init`                | Init the MQTT module (initializes the MQTT subsystem; call before W6X_MQTT_Configure / W6X_MQTT_Connect) |
| `W6X_MQTT_DeInit`              | De-Init the MQTT module.                                                                                 |
| `W6X_MQTT_SetRecvDataPtr`      | Set/change the pointer where to copy the Recv Data.                                                      |
| `W6X_MQTT_Configure`           | MQTT Set user configuration.                                                                             |
| `W6X_MQTT_Connect`             | MQTT Connect to broker.                                                                                  |
| `W6X_MQTT_GetConnectionStatus` | MQTT Get connection status.                                                                              |
| `W6X_MQTT_Disconnect`          | MQTT Disconnect from broker.                                                                             |
| `W6X_MQTT_Subscribe`           | MQTT Subscribe to a topic.                                                                               |
| `W6X_MQTT_GetSubscribedTopics` | MQTT Get subscribed topics.                                                                              |
| `W6X_MQTT_Unsubscribe`         | MQTT Unsubscribe from a topic.                                                                           |
| `W6X_MQTT_Publish`             | MQTT Publish a message to a topic (publishes the provided payload to the given topic)                    |
| `W6X_MQTT_StateToStr`          | Convert the MQTT state to a string.                                                                      |

### BLE API Functions

The following functions are available in the \ref ST67W6X_API_BLE_Public_Functions "BLE API group":

| Function Name                           | Description                                                                                                                                                                                                                                                  |
| --------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `W6X_Ble_GetInitMode`                   | Get BLE initialization mode.                                                                                                                                                                                                                                 |
| `W6X_Ble_Init`                          | Initialize BLE Server/Client/Dual mode (initializes the BLE subsystem in the requested role)                                                                                                                                                                 |
| `W6X_Ble_DeInit`                        | De-Initialize BLE Server/Client/Dual mode.                                                                                                                                                                                                                   |
| `W6X_Ble_SetRecvDataPtr`                | Set/change the pointer where to copy the Recv Data.                                                                                                                                                                                                          |
| `W6X_Ble_SetTxPower`                    | BLE Set TX Power.                                                                                                                                                                                                                                            |
| `W6X_Ble_GetTxPower`                    | BLE Get TX Power.                                                                                                                                                                                                                                            |
| `W6X_Ble_AdvStart`                      | BLE Server Start Advertising.                                                                                                                                                                                                                                |
| `W6X_Ble_AdvStop`                       | BLE Server Stop Advertising.                                                                                                                                                                                                                                 |
| `W6X_Ble_GetBDAddress`                  | Retrieves the BLE BD address.                                                                                                                                                                                                                                |
| `W6X_Ble_Disconnect`                    | Disconnect from a BLE remote device.                                                                                                                                                                                                                         |
| `W6X_Ble_ExchangeMTU`                   | Exchange BLE MTU length.                                                                                                                                                                                                                                     |
| `W6X_Ble_SetBdAddress`                  | Set BLE BD Address.                                                                                                                                                                                                                                          |
| `W6X_Ble_SetDeviceName`                 | Set BLE device Name.                                                                                                                                                                                                                                         |
| `W6X_Ble_GetDeviceName`                 | This function retrieves the BLE device name.                                                                                                                                                                                                                 |
| `W6X_Ble_SetAdvData`                    | Set BLE Advertising Data (maximum payload length is W6X_BLE_MAX_ADV_DATA_LENGTH bytes)                                                                                                                                                                       |
| `W6X_Ble_SetScanRespData`               | Set BLE scan response Data (maximum payload length is W6X_BLE_MAX_SCAN_RESP_DATA_LENGTH bytes)                                                                                                                                                               |
| `W6X_Ble_SetAdvParam`                   | Set BLE Advertising Parameters (advertising intervals are expressed in units of 0.625 ms)                                                                                                                                                                    |
| `W6X_Ble_StartScan`                     | Start BLE Device scan.                                                                                                                                                                                                                                       |
| `W6X_Ble_StopScan`                      | Stop BLE Device scan.                                                                                                                                                                                                                                        |
| `W6X_Ble_SetScanParam`                  | Set the BLE scan parameters (configures the GAP scanner used by W6X_Ble_StartScan; scan_interval/scan_window units are 0.625 ms; constraints: scan_window <= scan_interval, valid range [4 .. 0x4000] in 0.625 ms units; example: 50 ms -> scan_interval=80) |
| `W6X_Ble_GetScanParam`                  | Get the Scan parameters (scan_interval and scan_window are expressed in units of 0.625 ms)                                                                                                                                                                   |
| `W6X_Ble_Print_Scan`                    | Print the scan results.                                                                                                                                                                                                                                      |
| `W6X_Ble_GetAdvParam`                   | Get BLE Advertising Parameters.                                                                                                                                                                                                                              |
| `W6X_Ble_SetConnParam`                  | Set BLE Connection Parameters (all time parameters use BLE specification units)                                                                                                                                                                              |
| `W6X_Ble_GetConnParam`                  | Get the connection parameters.                                                                                                                                                                                                                               |
| `W6X_Ble_GetConn`                       | Get the connection information.                                                                                                                                                                                                                              |
| `W6X_Ble_Connect`                       | Create connection to a remote device.                                                                                                                                                                                                                        |
| `W6X_Ble_SetDataLength`                 | Set the BLE Data length.                                                                                                                                                                                                                                     |
| `W6X_Ble_CreateService`                 | Create BLE Service (service_index must be in range [0 .. (W6X_BLE_MAX_CREATED_SERVICE_NBR - 1)]; service_uuid is a null-terminated UUID string; maximum string storage size is W6X_BLE_MAX_UUID_SIZE bytes)                                                  |
| `W6X_Ble_DeleteService`                 | Delete BLE Service.                                                                                                                                                                                                                                          |
| `W6X_Ble_CreateCharacteristic`          | Create BLE Characteristic (char_index must be in range [0 .. (W6X_BLE_MAX_CHAR_NBR - 1)]; char_uuid is a null-terminated UUID string; maximum string storage size is W6X_BLE_MAX_UUID_SIZE bytes)                                                            |
| `W6X_Ble_GetServicesAndCharacteristics` | List BLE Services and their Characteristics (ServicesTable must provide space for at least W6X_BLE_MAX_CREATED_SERVICE_NBR services; each service can contain up to W6X_BLE_MAX_CHAR_NBR characteristics)                                                    |
| `W6X_Ble_RegisterCharacteristics`       | Register BLE characteristics.                                                                                                                                                                                                                                |
| `W6X_Ble_RemoteServiceDiscovery`        | Discover BLE services of remote device.                                                                                                                                                                                                                      |
| `W6X_Ble_RemoteCharDiscovery`           | Discover BLE Characteristics of a service remote device.                                                                                                                                                                                                     |
| `W6X_Ble_ServerNotify`                  | Notify the Characteristic Value from the Server to a Client (maximum notification/indication payload size is W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH bytes)                                                                                                        |
| `W6X_Ble_ServerIndicate`                | Indicate the Characteristic Value from the Server to a Client (maximum notification/indication payload size is W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH bytes)                                                                                                      |
| `W6X_Ble_ServerSetReadData`             | Set the data when Client read characteristic from the Server.                                                                                                                                                                                                |
| `W6X_Ble_ClientWriteData`               | Write data to a Server characteristic.                                                                                                                                                                                                                       |
| `W6X_Ble_ClientReadData`                | Read data from a Server characteristic.                                                                                                                                                                                                                      |
| `W6X_Ble_ClientSubscribeChar`           | Subscribe to notifications or indications from a Server characteristic.                                                                                                                                                                                      |
| `W6X_Ble_ClientUnsubscribeChar`         | Unsubscribe to notifications or indications from a Server characteristic.                                                                                                                                                                                    |
| `W6X_Ble_SetSecurityParam`              | Set BLE security parameters.                                                                                                                                                                                                                                 |
| `W6X_Ble_GetSecurityParam`              | Get BLE security parameters.                                                                                                                                                                                                                                 |
| `W6X_Ble_SecurityStart`                 | Start BLE security.                                                                                                                                                                                                                                          |
| `W6X_Ble_SecurityPassKeyConfirm`        | BLE security pass key confirm.                                                                                                                                                                                                                               |
| `W6X_Ble_SecurityPairingConfirm`        | BLE pairing confirm.                                                                                                                                                                                                                                         |
| `W6X_Ble_SecurityEnterRemotePassKey`    | BLE enter remote passkey.                                                                                                                                                                                                                                    |
| `W6X_Ble_SecurityPairingCancel`         | BLE pairing cancel.                                                                                                                                                                                                                                          |
| `W6X_Ble_SecurityUnpair`                | BLE unpair.                                                                                                                                                                                                                                                  |
| `W6X_Ble_SecurityGetBondedDeviceList`   | BLE get paired device list.                                                                                                                                                                                                                                  |
| `W6X_Ble_SetGapAppearance`              | BLE set GAP appearance.                                                                                                                                                                                                                                      |
| `W6X_Ble_RoleToStr`                     | Convert the BLE Connection Role to a string.                                                                                                                                                                                                                 |

### Netif API Functions

The following functions are available in the \ref ST67W6X_API_Netif_Public_Functions "Netif API group":

| Function Name      | Description                               |
| ------------------ | ----------------------------------------- |
| `W6X_Netif_Init`   | Initialize the Network Interface.         |
| `W6X_Netif_DeInit` | De-Initialize the Network Interface.      |
| `W6X_Netif_output` | Send data on the Network Interface.       |
| `W6X_Netif_input`  | Read data from the Network Interface.     |
| `W6X_Netif_free`   | Free internal buffer containing the data. |

## Utilities introduction

### Logging utility

The logging utility is used to manage the print traces over UART or ITM. The logging system is described here: https://www.freertos.org/logging.html

The logging macros are defined in the file `logging.h` and are:

| Macro         | Description                     |
| ------------- | ------------------------------- |
| LogError(...) | Print a critical error message  |
| LogWarn(...)  | Print an abnormal event message |
| LogInfo(...)  | Print an informational message  |
| LogDebug(...) | Print a debug message           |

A description of the semantic of the log levels is available here: https://github.com/FreeRTOS/FreeRTOS/blob/main/FreeRTOS-Plus/Source/Utilities/logging/logging_levels.h

For more information, see the \ref util_logging "Utility logging component" page.

### Shell utility

The shell utility is used to manage the command line interface over UART.

The shell macros are defined in the file `shell.h` and are:

| Macro                                      | Description                    |
| ------------------------------------------ | ------------------------------ |
| SHELL_FUNCTION_EXPORT_CMD(name, cmd, desc) | Export a function to the shell |
| SHELL_E(...)                               | Print an error message         |
| SHELL_CMD(...)                             | Define a shell command         |
| SHELL_PROMPT(...)                          | Print the shell prompt         |
| SHELL_PRINTF(...)                          | Print a formatted string       |
| SHELL_DBG(...)                             | Print a debug message          |

For more information, see the \ref util_shell "Utility shell component" page.

# Update history

## V1.3.0 / 10-April-2026

### Main changes

- **Wi-Fi**
  - ID P67W61-563: Accept receiving of deauthentication frames during authentication process
  - ID P67W61-632: Add TWT multiple flows capability
  - ID P67W61-657: Fix WPS connection stopped after 15s in T02 architecture
  - ID P67W61-690: Auto-connect is working after a successful WPS connection
  - ID P67W61-692: Fix the AT+CWMODE=1 command that could fail to stop the soft-AP sometimes
  - ID P67W61-693: RSSI update time decreased in low power
  - ID P67W61-723: Fix WiFi SetAntenna diversity with disabled mode
  - ID P67W61-731: Add missing netif callback SAP link up/down during Wi-Fi mode update
  - ID P67W61-742: Wi-Fi credentials stored in ST67W611M1 Flash used for the auto-reconnect are encrypted by a private key
  - ID P67W61-743: Add Wi-Fi APIs to set multiple AP credentials in secure storage and use them to connect without password on host

- **BLE**
  - ID P67W61-617: Default GAP Appearance value new set to 0. Implementation of a new API to modify this value.
  - ID P67W61-621: BLE Client Characteristic Configuration Descriptor of the notify characteristic are disabled by default
  - ID P67W61-663: Disable automatic BLE Advertising restart after disconnection
  - ID P67W61-706: Fix incorrect BLE Connection Role value in enum
  - ID P67W61-729: W6X_Ble_ServerSetReadData API now handles data lengths up to 240 bytes
  - ID P67W61-739: Rename "Set PassKey" API and related shell command to "Enter remote PassKey" to clarify functionality

- **Network** (ST67W611M T01 SW architecture only)
  - ID P67W61-613: Fix CIPSEND to drop (don't send) DATA exceeding the length specified in the parameter
  - ID P67W61-686: Fix AT+CIPSSLSCONF such to detect an error when crt param is not generated based on key param
  - ID P67W61-696: Allow to disable SNI from a socket to which SNI had been previously used (configured)
  - ID P67W61-765: Fix memory leak when opening several sockets using TLS protocol

- **HTTP**
  - None

- **MQTT**
  - ID P67W61-746: Update MQTT certificate and sni configuration on server with the mbedTLS3.6 component upgrade

- **Network interface** (ST67W611M T02 SW architecture only)
  - ID P67W61-656: Fix in UDPv6 server mode where +IPD event did not return the UDPv6 client IP address that sent data to the server
  - ID P67W61-699: Add defines to configure SPI Tx/Rx queues length

- **AT Parser**
  - None

- **System**
  - ID P67W61-683: Modify FreeRTOS xPortIsInsideInterrupt usage to stay compliant with old Kernel versions
  - ID P67W61-736: Check if SPI instance is initialized before to start the modem
  - ID P67W61-738: Fix overflow when SPI Tx buffer length is greater than Rx buffer allocated in spi_iface driver
  - ID P67W61-763: Fix warnings on strncpy length usage
  - ID P67W61-768: Fix modules status loss when the W6X_Reset function returns before the end of the reset sequence

- **Utilities**
  - ID P67W61-675: Add new shell operations
  - ID P67W61-678: Enhance Iperf UDP Server test end in T02 architecture
  - ID P67W61-689: Fix net pull empty socket data failure in Iperf

### Contents

- **Wi-Fi**
  - Station mode in Open/WEP/WPA2/WPA3 encryption in personal mode
  - Soft-AP mode in Open/WPA2/WPA3 encryption in personal mode
  - Station mode with WPS Push Button connection method
  - Static and Dynamic antenna diversity selection

- **BLE**
  - Server mode (10 connections max), Client mode (9 connections max) and Dual Mode (10 connections max with up to 9 as Client)
  - Up to 5 services (including default services) and 5 characteristics by service

- **Network** (ST67W611M T01 SW architecture only)
  - TCP/UDP/SSL socket service. 5 client connections max, 1 server connection max
  - IPv4 and IPv6 support

- **HTTP** (ST67W611M T01 SW architecture only)
  - HTTP/HTTPS Client over network sockets
  - GET/HEAD/POST/PUT support

- **MQTT**
  - MQTT/MQTTS Client service with one MQTT broker connection

- **Firmware updates**
  - Upgrade of ST67W611M Firmware Over-The-Air

- **Network interface** (ST67W611M T02 SW architecture only)
  - Network direct link to LwIP on Host with 2 link interfaces (Station and Soft-AP)

- **System**
  - ST67W611M Power save (standby and shutdown modes)

- **Utilities**
  - Shell service to call APIs as command line in UART terminal interface
  - Logging service to redirect traces on UART or ITM
  - Iperf service to evaluate network performances

### Known limitations

- Common architectures
  - W6X_WiFi_Connect API cannot use special characters [,\"\\] in the SSID and password. If needed, they must be preceded by a \\ to be interpreted correctly
  - W6X_Ble_SetDeviceName API cannot use special characters [,\"\\] in the device name. If needed, they must be preceded by a \\ to be interpreted correctly
  - W6X_Ble_SetBdAddress API doesn't support random BLE BD address

- T01 architecture
  - Wi-Fi communication in static IP does not work when power save is activated
  - Wi-Fi station does not answer to ARP requests by Access Point when static IP is used
  - Wi-Fi station may not receive an IP address after the DHCP sequence
  - Ping may fail after a while when Wi-Fi station is connected in WPA3
  - Once the DNS IP is set manually, those values will be hardcoded until the device is reset to its default configuration
  - Performance Rx throughput with Soft-AP shows high variance
  - W6X_MQTT_Configure API cannot use special characters [,"\\] in the username and password. If needed, they must be preceded by a \\ to be interpreted correctly
  - W6X_MQTT_Publish cannot send message larger than 1470 bytes

## V1.2.0 / 19-December-2025

### Main changes

- **Wi-Fi**
  - ID 214298: Add security and Wi-Fi protocol used during connection in W6X_WiFi_Station_GetState function
  - ID 217813: Add Wi-Fi antenna diversity automatic selection for -U and -P after Wi-Fi connection
  - ID 222980: Fix too restrictive timeout for WPS and disable powersave during connection

- **BLE**
  - ID 219085: Fix security cancel AT command
  - ID 221641: Add BLE Dual (Client and Server) Mode, and up to 10 connections max support
  - ID 223088: Fix Get BLE Connection API error returned when no connection detected

- **Network** (ST67W611M T01 SW architecture only)
  - ID 219808: Fix wfa tg echo command to be able to reuse port
  - ID 200391: Add IPv6 support and new IPv6 API while keeping IPv4 as the default solution
  - ID 214727: SSL client socket support timeout parameter when establishing a connection
  - ID 217059: Add timeout parameter on ping API and shell commands
  - ID 217197: Fix unexpected closing of new socket connection
  - ID 222582: Fix Iperf closing early and throughput degradation in UDPv6 server mode

- **HTTP**
  - None

- **MQTT**
  - ID 215159: Fix large MQTT messages restriction
  - ID 218963: Fix of QoS flags not taken into account in MQTT messages sent

- **Network interface** (ST67W611M T02 SW architecture only)
  - ID 215902: Add soft-AP support over LwIP (link capability)

- **AT Parser**
  - None

- **System**
  - ID 219266: Rename OTA API to FWU (FirmWare Update) to clarify usage

- **Utilities**
  - None

### Contents

- **Wi-Fi**
  - Station mode in Open/WEP/WPA2/WPA3 encryption in personal mode
  - Soft-AP mode in Open/WPA2/WPA3 encryption in personal mode
  - Station mode with WPS Push Button connection method
  - Static and Dynamic antenna diversity selection

- **BLE**
  - Server mode (10 connections max), Client mode (9 connections max) and Dual Mode (10 connections max with up to 9 as Client)
  - Up to 5 services (including default services) and 5 characteristics by service

- **Network** (ST67W611M T01 SW architecture only)
  - TCP/UDP/SSL socket service. 5 client connections max, 1 server connection max
  - IPv4 and IPv6 support

- **HTTP** (ST67W611M T01 SW architecture only)
  - HTTP/HTTPS Client over network sockets
  - GET/HEAD/POST/PUT support

- **MQTT**
  - MQTT/MQTTS Client service with one MQTT broker connection

- **Firmware updates**
  - Upgrade of ST67W611M Firmware Over-The-Air

- **Network interface** (ST67W611M T02 SW architecture only)
  - Network direct link to LwIP on Host with 2 link interfaces (Station and Soft-AP)

- **System**
  - ST67W611M Power save (standby and shutdown modes)

- **Utilities**
  - Shell service to call APIs as command line in UART terminal interface
  - Logging service to redirect traces on UART or ITM
  - Iperf service to evaluate network performances

### Known limitations

- Common architectures
  - W6X_WiFi_Connect API cannot use special characters [,\"\\] in the SSID and password. If needed, they must be preceded by a \\ to be interpreted correctly
  - Wi-Fi connection to an AP may fail if the previous connection to the same AP has not been ended by a disconnect
  - W6X_Ble_SetDeviceName API cannot use special characters [,\"\\] in the device name. If needed, they must be preceded by a \\ to be interpreted correctly
  - W6X_Ble_SetBdAddress API doesn't support random BLE BD address
  - No API to modify BLE GAP appearance value

- T01 architecture
  - Wi-Fi communication in static IP does not work when power save is activated
  - Wi-Fi station does not answer to ARP requests by Access Point when static IP is used
  - Wi-Fi station may not receive an IP address after the DHCP sequence
  - Ping may fail after a while when Wi-Fi station is connected in WPA3
  - Once the DNS IP is set manually, those values will be hardcoded until the device is reset to its default configuration
  - Unable to send data to a client in UDPv6 server mode
  - Performance Rx throughput with Soft-AP shows high variance
  - W6X_MQTT_Configure API cannot use special characters [,"\\] in the username and password. If needed, they must be preceded by a \\ to be interpreted correctly
  - W6X_MQTT_Publish cannot send message larger than 1470 bytes

- T02 architecture
  - Wi-Fi WPS connection is not working properly
  - After a long run of iperf as client, in TCP ipv4 mode, throughpout decreases to 0 Mbps

## V1.1.1 / 26-September-2025

### Main changes

- **Wi-Fi**
  - ID 210980: Fix wrong status message returned by scan command when several scans are sent in a row
  - ID 212466: TWT frames sent only to APs with TWT support
  - ID 213724: Maximum of Wi-Fi scan results reduced to 20 devices
  - ID 214286: Remove the IP address compatibility check from WiFi cb to dissociate WiFi module from Net module
  - ID 216134: The DTIM parameter used on Station side is now a factor of the Access Point DTIM, to be in phase with Access Point DTIM beacons
  - ID 216480: The soft Access Point protocol configuration is now a parameter when starting a soft Access Point. Station protocol configuration has been removed
  - ID 216882: Add Wi-Fi antenna diversity automatic selection for -U and -P after Wi-Fi connection

- **BLE**
  - ID 212742: Add BLE Shell commands to perform GATT and Security operations
  - ID 213724: Maximum of BLE scan results reduced to 20 devices
  - ID 215560: Add support of 2 BLE connections as Client
  - ID 215563: Report remote BD address type and Long Term Key (LTK) from BLE security events
  - ID 216043: BLE Init rework to ensure clock is correctly configured before entering in Low Power

- **Network** (ST67W611M T01 SW architecture only)
  - ID 210991: Improve Iperf RX throughput
  - ID 212413: Fix blocking state on reception when connecting multiple UDP sockets
  - ID 213424: Move IP/DNS/DHCP APIs from Wi-Fi to Net module to be compliant with LwIP on host structure project
  - ID 215289: Align the IP address format order in W6X_Net APIs on LwIP usage
  - ID 215983: Fix IPv6 and IPv4 string length being too small

- **HTTP** (ST67W611M T01 SW architecture only)
  - ID 211666: Increase SNI size for HTTP and choose server name as hostname when available
  - ID 216816: Added the ability to send HTTP requests larger than W61_MAX_SPI_XFER
  - ID 216997: Fix incorrect ALPN format for HTTPS API
  - ID 217079: Fix case sensitive connection: close value in preformatted HTTP client headers

- **MQTT**
  - ID 212696: Add support of MQTT raw data format
  - ID 214875: Add support of empty MQTT message for retain feature
  - ID 216425: Add support of QoS, Retain and LWT options in MQTT APIs

- **Network interface** (ST67W611M T02 SW architecture only)
  - ID 211434: Add support of LwIP on host based on ST67W611M T02 architecture

- **AT Parser**
  - ID 214401: Integration of AT parser based on Zephyr modem cmd handler
  - ID 215005: Remove maximum number of parameters limitation expected from NCP in AT parser to ensure backward compatibility even if another field is added

- **System**
  - ID 214011: Offer capability to disable assert macros
  - ID 214447: Display Module ID in banner based on Part Number
  - ID 215588: Use the memcpy32 custom implementation only in spi_iface to prevent misalignment issue with limited cortex architectures
  - ID 215597: RAM usage optimization in ST67W6X_Network_Driver code
  - ID 216013: Modification of the W6X_FS_WriteFile API to use a certificate stored in the application or in the LFS Host
  - ID 217233: Change default MAX_SPI_XFER from 6000 to 1520

- **Utilities**
  - ID 212675: Add the possibility to the user to disable the logs for each module of the Network Driver (Wi-Fi, NET, BLE, MQTT)
  - ID 214193: Addition of a define (SHELL_CMD_LEVEL) to activate all or a reduced list of shell commands
  - ID 216118: Remove HAL UART dependency of shell service

### Contents

- **Wi-Fi**
  - Station mode in Open/WEP/WPA2/WPA3 encryption in personal mode
  - Soft-AP mode in Open/WPA2/WPA3 encryption in personal mode
  - Station mode with WPS Push Button connection method

- **BLE**
  - Server mode (1 connection max) and Client mode (2 connections max)
  - Up to 5 services (including default services) and 5 characteristics by service

- **Network** (ST67W611M T01 SW architecture only)
  - TCP/UDP/SSL socket service. 5 client connections max, 1 server connection max

- **HTTP** (ST67W611M T01 SW architecture only)
  - HTTP/HTTPS Client over network sockets
  - GET/HEAD/POST/PUT support

- **MQTT**
  - MQTT/MQTTS Client service with one MQTT broker connection

- **Firmware updates**
  - Upgrade of ST67W611M Firmware Over-The-Air

- **Network interface** (ST67W611M T02 SW architecture only)
  - Network direct link to LwIP on Host with 2 link interfaces (Station and Soft-AP)

- **System**
  - ST67W611M Power save (standby and shutdown modes)

- **Utilities**
  - Shell service to call APIs as command line in UART terminal interface
  - Logging service to redirect traces on UART or ITM
  - Iperf service to evaluate network performances

### Known limitations

- Wi-Fi communication in static IP does not work when power save is activated
- Wi-Fi station does not answer to ARP requests by Access Point when static IP is used
- W6X_WiFi_Connect API cannot use special characters [,\"\\] in the SSID and password. If needed, they must be preceded by a \\ to be interpreted correctly
- Dynamic mode of W6X_WiFi_SetAntennaDiversity API is not functional in current release
- W6X_Ble_SetDeviceName API cannot use special characters [,\"\\] in the device name. If needed, they must be preceded by a \\ to be interpreted correctly
- Once the DNS IP is set manually, those values will be hardcoded until the device is reset to its default configuration
- SSL sockets support a limited amount of algorithms, handshake might fail if unsupported algorithm is used by the server
- W6X_MQTT_Configure API cannot use special characters [,\"\\] in the username and password. If needed, they must be preceded by a \\ to be interpreted correctly
- W6X_MQTT_Publish cannot send message larger than 1470 bytes

## V1.1.0 / 05-August-2025

### Main changes

- None

### Contents

- **Wi-Fi**
  - Station mode in Open/WEP/WPA2/WPA3 encryption in personal mode
  - Soft-AP mode in Open/WPA2/WPA3 encryption in personal mode
  - Station mode with WPS Push Button connection method

- **BLE**
  - Server mode (1 connection max) and Client mode (2 connections max)
  - Up to 5 services (including default services) and 5 characteristics by service

- **Network**
  - TCP/UDP/SSL socket service. 5 client connections max, 1 server connection max

- **HTTP**
  - HTTP/HTTPS Client over network sockets
  - GET/HEAD/POST/PUT support

- **MQTT**
  - MQTT/MQTTS Client service with one MQTT broker connection

- **Firmware updates**
  - Upgrade of ST67W611M Firmware Over-The-Air

- **System**
  - ST67W611M Power save (standby and shutdown modes)

- **Utilities**
  - Shell service to call APIs as command line in UART terminal interface
  - Logging service to redirect traces on UART or ITM
  - Iperf service to evaluate network performances

### Known limitations

- Wi-Fi communication in static IP does not work when power save is activated
- Wi-Fi station does not answer to ARP requests by Access Point when static IP is used
- Wi-Fi scan command is ignored immediately after a boot if auto-connect is enabled
- SSL sockets support a limited amount of algorithms, handshake might fail if unsupported algorithm is used by the server
- Performance Rx throughput with Soft-AP is instable
- Performance UDP Rx throughput is below expectation (expected 14 Mbps, actual 12 Mbps)

## V1.0.0 / 28-May-2025

### Main changes

- First official release

### Contents

- **Wi-Fi**
  - Station mode in Open/WEP/WPA2/WPA3 encryption in personal mode
  - Soft-AP mode in Open/WPA2/WPA3 encryption in personal mode
  - Station mode with WPS Push Button connection method

- **BLE**
  - Server mode (1 connection max) and Client mode (2 connections max)
  - Up to 5 services (including default services) and 5 characteristics by service

- **Network**
  - TCP/UDP/SSL socket service. 5 client connections max, 1 server connection max

- **HTTP**
  - HTTP/HTTPS Client over network sockets
  - GET/HEAD/POST/PUT support

- **MQTT**
  - MQTT/MQTTS Client service with one MQTT broker connection

- **Firmware updates**
  - Upgrade of ST67W611M Firmware Over-The-Air

- **System**
  - ST67W611M Power save (standby and shutdown modes)

- **Utilities**
  - Shell service to call APIs as command line in UART terminal interface
  - Logging service to redirect traces on UART or ITM
  - Iperf service to evaluate network performances

### Known limitations

- Wi-Fi communication in static IP does not work when power save is activated
- Wi-Fi station does not answer to ARP requests by Access Point when static IP is used
- Wi-Fi scan command is ignored immediately after a boot if auto-connect is enabled
- SSL sockets support a limited amount of algorithms, handshake might fail if unsupported algorithm is used by the server
- Performance Rx throughput with Soft-AP is instable
- Performance UDP Rx throughput is below expectation (expected 14 Mbps, actual 12 Mbps)

# License

See [LICENSE](../LICENSE.md) for more info.
