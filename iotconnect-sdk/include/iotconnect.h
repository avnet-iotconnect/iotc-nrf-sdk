//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by nmarkovi on 6/15/20.
//

#ifndef IOTCONNECT_H
#define IOTCONNECT_H

#include <stddef.h>
#include "iotconnect_event.h"
#include "iotconnect_telemetry.h"
#include "iotconnect_lib.h"

#define HTTPS_PORT 443
#define MAXLINE 2048

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UNDEFINED,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_FAILED,
    // TODO: Sync statuses etc.
} IOT_CONNECT_STATUS;


typedef void (*IOT_CONNECT_STATUS_CB)(IOT_CONNECT_STATUS data);

typedef struct {

    char *env;    // Environment name. Contact your representative for details.
    char *cpid;   // Settings -> Company Profile.
    char *duid;   // Name of the device.
    // TODO: Support different kinds of auth
    int auth_type; // currently ignored
    union { // union because we may support different types of auth
        const char *client_private_key; // PEM format (with newlines). Pointer may be freed init.
        const char *client_certificate; // PEM format (with newlines). Pointer may be freed init.
    } auth;
    IOTCL_OTA_CALLBACK ota_cb; // callback for OTA events.
    IOTCL_COMMAND_CALLBACK cmd_cb; // callback for command events.
    IOTCL_MESSAGE_CALLBACK msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IOT_CONNECT_STATUS_CB status_cb; // callback for connection status
} IOTCONNECT_CLIENT_CONFIG;


IOTCONNECT_CLIENT_CONFIG *IotConnectSdk_GetConfig();

int IotConnectSdk_Init();

bool IotConnectSdk_IsConnected();

IOTCL_CONFIG *IotConnectSdk_GetLibConfig();

void IotConnectSdk_SendPacket(const char *data);

void IotConnectSdk_Loop();

void IotConnectSdk_Disconnect();

#ifdef __cplusplus
}
#endif

#endif
