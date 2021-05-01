//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by nmarkovi on 6/15/20.
// Modified by Alan Low <alan.low@avnet.com> on 4/28/21.
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

typedef void (*IOT_CONNECT_MQTT_ON_PUBACK_TIMEOUT_CB)(uint32_t msg_id);

typedef struct {

    uint32_t timeout_s;	// publish acknowledge timeout in seconds.
    IOT_CONNECT_MQTT_ON_PUBACK_TIMEOUT_CB cb; //callback for publish acknowledge timeout.

} IOTCONNECT_MQTT_PUBACK_TIMEOUT_CONFIG;

typedef struct {
    int tls_verify; // NONE = 0, OPTIONAL = 1, REQUIRED = 2
    const char *env; // Environment name. Contact your representative for details. Same as telemetry config.
    IOT_CONNECT_MQT_ON_DATA_CB data_cb; // callback for OTA events.
    IOT_CONNECT_STATUS_CB status_cb; // callback for command events.
    IOTCONNECT_MQTT_PUBACK_TIMEOUT_CONFIG puback_timeout_cfg; //puback timeout config.
} IOTCONNECT_MQTT_CONFIG;

bool iotc_nrf_mqtt_init(IOTCONNECT_MQTT_CONFIG *config, IOTCL_SyncResponse* sr);

void iotc_nrf_mqtt_loop();

bool iotc_nrf_mqtt_is_connected();

int iotc_nrf_mqtt_publish(struct mqtt_client *c, const char *topic, enum mqtt_qos qos, const uint8_t *data, size_t len,
                            uint32_t *msg_id);

void iotc_nrf_mqtt_disconnect();

void iotc_nrf_mqtt_abort();

#ifdef __cplusplus
}
#endif
#endif //IOTCONNECT_ZEPHYR_MQTT_H
