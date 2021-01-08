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

typedef void (*IOT_CONNECT_MQT_ON_DATA_CB)(const uint8_t *data, size_t len, const char *topic);

typedef struct {
    int tls_verify; // NONE = 0, OPTIONAL = 1, REQUIRED = 2
    const char *env; // Environment name. Contact your representative for details. Same as telemetry config.
    IOT_CONNECT_MQT_ON_DATA_CB data_cb; // callback for OTA events.
    IOT_CONNECT_STATUS_CB status_cb; // callback for command events.
} IOTCONNECT_MQTT_CONFIG;

bool iotc_nrf_mqtt_init(IOTCONNECT_MQTT_CONFIG *config, IOTCL_SyncResponse* sr);

void iotc_nrf_mqtt_loop();

bool iotc_nrf_mqtt_is_connected();

int iotc_nrf_mqtt_publish(struct mqtt_client *c, const char *topic, enum mqtt_qos qos, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
#endif //IOTCONNECT_ZEPHYR_MQTT_H
