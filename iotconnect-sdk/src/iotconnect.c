//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/15/20.
//
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <cJSON.h>

#include <zephyr.h>
#include <iotconnect_socket_https.h>

#include "nrf_cert_store.h"
#include "iotconnect_socket_https.h"

// override IOTCONNECT_DISCOVERY_HOSTNAME BEFORE including any iotconnect includes in case the user overrides it in config
#define IOTCONNECT_DISCOVERY_HOSTNAME CONFIG_DISCOVERY_HOSTNAME

#include "iotconnect_discovery.h"
#include "iotconnect_event.h"

#include "iotconnect_mqtt.h"
#include "iotconnect.h"


/* Buffers for MQTT client. */
IotclDiscoveryResponse *discovery_response = NULL;
IotclSyncResponse *sync_response = NULL;
struct mqtt_client client;

static IotconnectClientConfig config = {0};
static IotclConfig lib_config = {0};;
static IotconnectMqttConfig mqtt_config = {0};

static char send_buf[MAXLINE + 1];

static void dump_response(const char *message, IotconnectNrfHttpResponse *response) {
    printk("%s", message);
    if (response->raw_response) {
        printk(" Response was:\n----\n%s\n----\n", response->raw_response);
    } else {
        printk(" Response was empty\n");
    }
}

static void report_sync_error(IotclSyncResponse *response, const char *sync_response_str) {
    if (NULL == response) {
        printk("Failed to obtain sync response?\n");
        return;
    }
    switch (response->ds) {
        case IOTCL_SR_DEVICE_NOT_REGISTERED:
            printk("IOTC_SyncResponse error: Not registered\n");
            break;
        case IOTCL_SR_AUTO_REGISTER:
            printk("IOTC_SyncResponse error: Auto Register\n");
            break;
        case IOTCL_SR_DEVICE_NOT_FOUND:
            printk("IOTC_SyncResponse error: Device not found\n");
            break;
        case IOTCL_SR_DEVICE_INACTIVE:
            printk("IOTC_SyncResponse error: Device inactive\n");
            break;
        case IOTCL_SR_DEVICE_MOVED:
            printk("IOTC_SyncResponse error: Device moved\n");
            break;
        case IOTCL_SR_CPID_NOT_FOUND:
            printk("IOTC_SyncResponse error: CPID not found\n");
            break;
        case IOTCL_SR_UNKNOWN_DEVICE_STATUS:
            printk("IOTC_SyncResponse error: Unknown device status error from server\n");
            break;
        case IOTCL_SR_ALLOCATION_ERROR:
            printk("IOTC_SyncResponse internal error: Allocation Error\n");
            break;
        case IOTCL_SR_PARSING_ERROR:
            printk("IOTC_SyncResponse internal error: Parsing error. Please check parameters passed to the request.\n");
            break;
        default:
            printk("WARN: report_sync_error called, but no error returned?\n");
            break;
    }
    printk("Raw server response was:\n--------------\n%s\n--------------\n", sync_response_str);
}

static IotclDiscoveryResponse *run_http_discovery(const char *cpid, const char *env) {
    IotclDiscoveryResponse *ret = NULL;

    int http_head_len = snprintk(send_buf,
                                 MAXLINE, /*total length should not exceed MTU size*/
    IOTCONNECT_DISCOVERY_HEADER_TEMPLATE, cpid, env
    );
    send_buf[http_head_len] = 0;
    IotconnectNrfHttpResponse response;
    iotconnect_https_request(&response,
                             IOTCONNECT_DISCOVERY_HOSTNAME,
                             TLS_SEC_TAG_IOTCONNECT_API,
                             send_buf
    );

    if (NULL == response.data) {
        dump_response("Unable to parse HTTP response,", &response);
        goto cleanup;
    }
    char *json_start = strstr(response.data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.", &response);
        goto cleanup;
    }
    if (json_start != response.data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.", &response);
    }

    ret = iotcl_discovery_parse_discovery_response(json_start);

    cleanup:
    iotconnect_free_https_response(&response);
    // fall through
    return ret;


}

static IotclSyncResponse *run_http_sync(const char *cpid, const char *uniqueid) {
    IotclSyncResponse *ret = NULL;
    char post_data[IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN + 1] = {0};
    snprintk(post_data,
             IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN, /*total length should not exceed MTU size*/
             IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_TEMPLATE,
             cpid,
             uniqueid
    );

    int http_post_len = snprintk(send_buf,
                                 1024, /*total length should not exceed MTU size*/
                                 IOTCONNECT_SYNC_HEADER_TEMPLATE, discovery_response->path, discovery_response->host,
                                 strlen(post_data), post_data
    );
    send_buf[http_post_len] = 0;
    IotconnectNrfHttpResponse response;
    iotconnect_https_request(&response,
                             discovery_response->host,
                             TLS_SEC_TAG_IOTCONNECT_API,
                             send_buf
    );

    if (NULL == response.data) {
        dump_response("Unable to parse HTTP response.", &response);
        goto cleanup;
    }
    char *json_start = strstr(response.data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.", &response);
        goto cleanup;
    }
    if (json_start != response.data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.", &response);
    }

    ret = iotcl_discovery_parse_sync_response(json_start);
    if (!ret || ret->ds != IOTCL_SR_OK) {
        report_sync_error(ret, response.raw_response);
        iotcl_discovery_free_sync_response(ret);
        ret = NULL;
    }

    cleanup:
    iotconnect_free_https_response(&response);
    // fall through

    return ret;
}

// this function will Give you Device CallBack payload
void iotc_on_mqtt_data(const uint8_t *data, size_t len, const char *topic) {
    (void) topic; // ignore topic for now
    char *str = malloc(len + 1);
    memcpy(str, data, len);
    str[len] = 0;
    printk("event>>> %s\n", str);
    if (!iotcl_process_event(str)) {
        printk("Error encountered while processing %s\n", str);
    }
    free(str);
}

static void on_iotconnect_status(IotconnectConnectionStatus status) {
    if (config.status_cb) {
        config.status_cb(status);
    }
}


///////////////////////////////////////////////////////////////////////////////////
// Get All twin property from C2D
void iotconnect_sdk_disconnect() {
    printk("Disconnecting...\n");
    mqtt_disconnect(&client);
    k_msleep(100);
}

void iotconnect_sdk_send_packet(const char *data) {
    if (0 != iotc_nrf_mqtt_publish(&client, sync_response->broker.pub_topic, 1, data, strlen(data))) {
        printk("\n\t Device_Attributes_Data Publish failure");
    }
}

static void on_message_intercept(IotclEventData data, IotConnectEventType type) {
    switch (type) {
        case ON_FORCE_SYNC:
            iotconnect_sdk_disconnect();
            iotcl_discovery_free_discovery_response(discovery_response);
            iotcl_discovery_free_sync_response(sync_response);
            sync_response = NULL;
            discovery_response = run_http_discovery(config.cpid, config.env);
            if (NULL == discovery_response) {
                printk("Unable to run HTTP discovery on ON_FORCE_SYNC \n");
                return;
            }
            sync_response = run_http_sync(config.cpid, config.duid);
            if (NULL == sync_response) {
                printk("Unable to run HTTP sync on ON_FORCE_SYNC \n");
                return;
            }
            (void) iotc_nrf_mqtt_init(&mqtt_config, sync_response);
        case ON_CLOSE:
            printk("Got a disconnect request. Closing the mqtt connection. Device restart is required.\n");
            iotconnect_sdk_disconnect();
        default:
            break; // not handling nay other messages
    }

    if (NULL != config.msg_cb) {
        config.msg_cb(data, type);
    }
}

void iotconnect_sdk_loop() {
    iotc_nrf_mqtt_loop();
}

IotclConfig *iotconnect_sdk_get_lib_config() {
    return iotcl_get_config();
}

IotconnectClientConfig *iotconnect_sdk_init_and_get_config() {
    memset(&config, 0, sizeof(config));
    return &config;
}

bool iotconnect_sdk_is_connected() {
    return iotc_nrf_mqtt_is_connected();
}


///////////////////////////////////////////////////////////////////////////////////
// this the Initialization os IoTConnect SDK
int iotconnect_sdk_init() {
    if (!discovery_response) {
        discovery_response = run_http_discovery(config.cpid, config.env);
        if (NULL == discovery_response) {
            // get_base_url will print the error
            return -1;
        }
    }

    if (!sync_response) {
        sync_response = run_http_sync(config.cpid, config.duid);
        if (NULL == sync_response) {
            // Sync_call will print the error
            return -2;
        }
    }

    // We want to print only first 4 characters of cpid. %.4s doesn't seem to work with prink
    char cpid_buff[5];
    strncpy(cpid_buff, sync_response->cpid, 4);
    cpid_buff[4] = 0;
    printk("CPID: %s***\n", cpid_buff);
    printk("ENV:  %s\n", config.env);

    mqtt_config.tls_verify = CONFIG_PEER_VERIFY;
    mqtt_config.data_cb = iotc_on_mqtt_data;
    mqtt_config.status_cb = on_iotconnect_status;

    if (!iotc_nrf_mqtt_init(&mqtt_config, sync_response)) {
        return -4;
    }

    lib_config.device.env = config.env;
    lib_config.device.cpid = config.cpid;
    lib_config.device.duid = config.duid;
    lib_config.telemetry.dtg = sync_response->dtg;;
    lib_config.event_functions.ota_cb = config.ota_cb;
    lib_config.event_functions.cmd_cb = config.cmd_cb;

    // intercept internal processing and forward to client
    lib_config.event_functions.msg_cb = on_message_intercept;

    if (!iotcl_init(&lib_config)) {
        printk("Failed to initialize the IoTConnect Lib");
    }

    return 0;
}