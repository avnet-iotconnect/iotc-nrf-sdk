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
} IotconnectConnectionStatus;


typedef void (*IotConnectStatusCallback)(IotconnectConnectionStatus data);

typedef struct {
    char *env;    // Environment name. Contact your representative for details.
    char *cpid;   // Settings -> Company Profile.
    char *duid;   // Name of the device.
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotclMessageCallback msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IotConnectStatusCallback status_cb; // callback for connection status
} IotconnectClientConfig;


IotconnectClientConfig *iotconnect_sdk_init_and_get_config();

int iotconnect_sdk_init();

bool iotconnect_sdk_is_connected();

IotclConfig *iotconnect_sdk_get_lib_config();

void iotconnect_sdk_send_packet(const char *data);

void iotconnect_sdk_loop();

void iotconnect_sdk_disconnect();

#ifdef __cplusplus
}
#endif

#endif
