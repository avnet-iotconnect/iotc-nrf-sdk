//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by nmarkovi on 6/15/20.
//


#ifndef IOTCONNECT_MQTT_H
#define IOTCONNECT_MQTT_H

#include <stddef.h>

#include "iotconnect_discovery.h"
#include "iotconnect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*IotconnectMqttOnDataCallback)(const uint8_t *data, size_t len, const char *topic);

typedef struct {
    int tls_verify; // NONE = 0, OPTIONAL = 1, REQUIRED = 2
    const char *env; // Environment name. Contact your representative for details. Same as telemetry config.
    IotconnectMqttOnDataCallback data_cb; // callback for OTA events.
    IotConnectStatusCallback status_cb; // callback for command events.
    IotConnectMsgSendStatusCallback msg_send_status_cb;  // callback for message send status.
} IotconnectMqttConfig;

bool iotc_nrf_mqtt_init(IotconnectMqttConfig *config, IotclSyncResponse *sr);

void iotc_nrf_mqtt_loop();

bool iotc_nrf_mqtt_is_connected();

int iotc_nrf_mqtt_publish(struct mqtt_client *c, const char *topic, enum mqtt_qos qos, const uint8_t *data,
                            size_t len, uint32_t *p_msg_id);

void iotc_nrf_mqtt_abort();

#ifdef __cplusplus
}
#endif
#endif //IOTCONNECT_ZEPHYR_MQTT_H
