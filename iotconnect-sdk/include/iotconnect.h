//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by nmarkovi on 6/15/20.
// Modified by Alan Low <alan.low@avnet.com> on 4/28/21.
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

typedef void (*IOT_CONNECT_MSG_ACK_TIMEOUT_CB)(uint32_t msg_id);

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
    IOT_CONNECT_MSG_ACK_TIMEOUT_CB msg_ack_timeout_cb; //callback for message acknowledge timeout.
} IOTCONNECT_CLIENT_CONFIG;


IOTCONNECT_CLIENT_CONFIG *IotConnectSdk_InitAndGetConfig();

int IotConnectSdk_Init();

bool IotConnectSdk_IsConnected();

IOTCL_CONFIG *IotConnectSdk_GetLibConfig();

bool IotConnectSdk_SendPacket(const char *data, uint32_t *msg_id);

void IotConnectSdk_Loop();

void IotConnectSdk_Disconnect();

int IotConnectSdk_Connect();

int IotConnectSdk_Abort();

#ifdef __cplusplus
}
#endif

#endif
