//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/15/20.
//
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <net/socket.h>
#include <cJSON.h>

#include <net/mqtt.h>
#include <net/http_client.h>
#include <zephyr.h>

#include "nrf_cert_store.h"

// override IOTCONNECT_DISCOVERY_HOSTNAME BEFORE including any iotconnect includes in case the user overrides it in config
#define IOTCONNECT_DISCOVERY_HOSTNAME CONFIG_DISCOVERY_HOSTNAME
#include "iotconnect_discovery.h"
#include "iotconnect_event.h"

#include "iotconnect_mqtt.h"
#include "iotconnect.h"


/* Buffers for MQTT client. */
IOTCL_DiscoveryResponse *discovery_response = NULL;
IOTCL_SyncResponse *sync_response = NULL;
struct mqtt_client client;

static IOTCONNECT_CLIENT_CONFIG config = {0};
static IOTCL_CONFIG lib_config = {0};;
static IOTCONNECT_MQTT_CONFIG mqtt_config = {0};

static char recv_buf[MAXLINE+1];
static char send_buf[MAXLINE+1];

static void report_sync_error(IOTCL_SyncResponse *response, const char* sync_response_str) {
    if (NULL == response) {
        printk("IOTC_SyncResponse is NULL. Out of memory?\n");
        return;
    }
    switch(response->ds) {
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

static IOTCL_DiscoveryResponse *run_http_discovery(const char *cpid, const char *env) {
    IOTCL_DiscoveryResponse *ret = NULL;
    int err;
    int fd = -1;

    struct addrinfo *res = NULL;
    struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
    };

    err = getaddrinfo(CONFIG_DISCOVERY_HOSTNAME, NULL, &hints, &res);
    if (err) {
        printk("Unable to resolve host\n");
        goto clean_up;
    }

    ((struct sockaddr_in *) res->ai_addr)->sin_port = htons(HTTPS_PORT);

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

    if (fd < 0) {
        printk("Failed to open socket!\n");
        goto clean_up;
    }

    err = NrfCertStore_ConfigureApiFd(fd);
    if (err) {
        goto clean_up;
    }
    struct sockaddr_in * a = (struct sockaddr_in *)(res->ai_addr);
    printk("Connecting to %s %d.%d.%d.%d ... ", CONFIG_DISCOVERY_HOSTNAME,
           a->sin_addr.s4_addr[0],
           a->sin_addr.s4_addr[1],
           a->sin_addr.s4_addr[2],
           a->sin_addr.s4_addr[3]
            );
    err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
    if (err) {
        printk("connect() failed, err: %d\n", errno);
        goto clean_up;
    }
    printk("OK\n");
    int HTTP_HEAD_LEN = snprintk(send_buf,
                                 MAXLINE, /*total length should not exceed MTU size*/
                                 IOTCONNECT_DISCOVERY_HEADER_TEMPLATE, cpid, env
    );

    int bytes;
    size_t off = 0;
    do {
        bytes = send(fd, &send_buf[off], HTTP_HEAD_LEN - off, 0);
        if (bytes < 0) {
            printk("send() failed, err %d\n", errno);
            goto clean_up;
        }
        off += bytes;
    } while (off < HTTP_HEAD_LEN);

    off = 0;
    do {
        bytes = recv(fd, &recv_buf[off], MAXLINE - off, 0);
        if (bytes < 0) {
            printk("recv() failed, err %d\n", errno);
            goto clean_up;
        }
        off += bytes;
    } while (bytes != 0);

    recv_buf[off] = 0;

    if (off == 0) {
        printk("Got empty response from server\n");
        goto clean_up;
    }

    char *json_start = strstr(recv_buf, "\r\n{");
    if (NULL == json_start) {
        printk("No json response from server. Response was:\n----\n%s\n----\n", recv_buf);
        goto clean_up;
    }

    ret = IOTC_DiscoveryParseDiscoveryResponse(json_start);
    // fall through
    clean_up:
    if (fd >= 0) {
        close(fd);
    }
    freeaddrinfo(res);
    return ret;
}

static char *run_http_sync(const char *cpid, const char *uniqueid) {
    char *sync_resp = NULL;
    struct addrinfo *res = NULL;
    int err;
    int fd = -1;
    size_t off;

    //printk("host:%s path:%s\n", host, path);

    struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
    };
    err = getaddrinfo(discovery_response->host, NULL, &hints, &res);
    if (err) {
        printk("Unable to resolve host");
        goto clean_up;
    }

    ((struct sockaddr_in *) res->ai_addr)->sin_port = htons(HTTPS_PORT);

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

    if (fd == -1) {
        printk("Failed to open socket!\n");
        goto clean_up;
    }

    err = NrfCertStore_ConfigureApiFd(fd);
    if (err) {
        goto clean_up;
    }

    struct sockaddr_in * a = (struct sockaddr_in *)(res->ai_addr);
    printk("Connecting to %s %d.%d.%d.%d ... ", discovery_response->host,
           a->sin_addr.s4_addr[0],
           a->sin_addr.s4_addr[1],
           a->sin_addr.s4_addr[2],
           a->sin_addr.s4_addr[3]
    );

    err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
    if (err) {
        printk("connect() failed, err: %d\n", errno);
        goto clean_up;
    }
    printk("OK\n");
    char post_data[IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN + 1] = {0};
    snprintk(post_data,
             IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN, /*total length should not exceed MTU size*/
             IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_TEMPLATE,
             cpid,
             uniqueid
    );

    int HTTP_POST_LEN = snprintk(send_buf,
                                 1024, /*total length should not exceed MTU size*/
                                 IOTCONNECT_SYNC_HEADER_TEMPLATE, discovery_response->path, discovery_response->host,
                                 strlen(post_data), post_data
    );
    off = 0;  //
    //printk("send_buf: %s\n", send_buf);
    do {
        int bytes = send(fd, &send_buf[off], HTTP_POST_LEN - off, 0);
        if (bytes < 0) {
            printk("send() failed, err %d\n", errno);
            goto clean_up;
        }
        off += bytes;
    } while (off < HTTP_POST_LEN);

    //printk("buffer sent!\n");

    off = 0;
    int bytes;
    do {
        bytes = recv(fd, &recv_buf[off], MAXLINE - off, 0);
        if (bytes < 0) {
            printk("recv() failed, err %d\n", errno);
            goto clean_up;
        }
        off += bytes;
        //printk("got %d bytes\n", bytes);
    } while (bytes != 0 /* peer closed connection */);
    recv_buf[off] = 0;
    //printk("Sync response: %s", recv_buf);
    //sync_resp = strstr(recv_buf, "\r\n{");

    if (off == 0) {
        printk("Got empty response from server\n");
        goto clean_up;
    }

    char *json_start = strstr(recv_buf, "\r\n{");
    if (NULL == json_start) {
        printk("No json response from server. Response was:\n----\n%s\n----\n", recv_buf);
        goto clean_up;
    }

    freeaddrinfo(res);
    close(fd);
    return json_start;

    // fall through
    clean_up:
    if (res) {
        freeaddrinfo(res);
    }
    if (fd >= 0) {
        close(fd);
    }

    return sync_resp;
}

// this function will Give you Device CallBack payload
void iotc_on_mqtt_data(const uint8_t *data, size_t len, const char *topic) {
    (void) topic; // ignore topic for now
    char *str = malloc(len + 1);
    memcpy(str, data, len);
    str[len] = 0;
    printk("event>>> %s\n", str);
    if (!IOTCL_ProcessEvent(str)) {
        printk("Error encountered while processing %s\n", str);
    }
    free(str);
}

static void on_iotconnect_status(IOT_CONNECT_STATUS status) {
    if (config.status_cb) {
        config.status_cb(status);
    }
}


///////////////////////////////////////////////////////////////////////////////////
// Get All twin property from C2D
void IotConnectSdk_Disconnect() {
    printk("Disconnecting...\n");
    mqtt_disconnect(&client);
    k_msleep(100);
    IOTCL_DiscoveryFreeDiscoveryResponse(discovery_response);
    IOTCL_DiscoveryFreeSyncResponse(sync_response);
    discovery_response = NULL;
    sync_response = NULL;
}

void IotConnectSdk_SendPacket(const char *data) {
    if (0 != iotc_nrf_mqtt_publish(&client, sync_response->broker.pub_topic, 1, data, strlen(data))) {
        printk("\n\t Device_Attributes_Data Publish failure");
    }
}

static void on_message_intercept(IOTCL_EVENT_DATA data, IotConnectEventType type) {
    switch (type) {
        case ON_FORCE_SYNC:
            IotConnectSdk_Disconnect();
            char *sync_resp_str = run_http_sync(config.cpid, config.duid);
            if (NULL == sync_resp_str) {
                printk("Failed to initialize the SDK\n");
                break;
            }
            sync_response = IOTCL_DiscoveryParseSyncResponse(sync_resp_str);
            if (!sync_response || sync_response->ds != IOTCL_SR_OK) {
                report_sync_error(sync_response, sync_resp_str);
                IOTCL_DiscoveryFreeSyncResponse(sync_response);
                sync_response = NULL;
                free(sync_resp_str);
                return;
            }
            (void) iotc_nrf_mqtt_init(&mqtt_config, sync_response);
        case ON_CLOSE:
            printk("Got a disconnect request. Closing the mqtt connection. Device restart is required.\n");
            IotConnectSdk_Disconnect();
        default:
            break; // not handling nay other messages
    }

    if (NULL != config.msg_cb) {
        config.msg_cb(data, type);
    }
}

void IotConnectSdk_Loop() {
    iotc_nrf_mqtt_loop();
}

IOTCL_CONFIG *IotConnectSdk_GetLibConfig() {
    return IOTCL_GetConfig();
}

IOTCONNECT_CLIENT_CONFIG *IotConnectSdk_GetConfig() {
    memset(&config, 0, sizeof(config));
    return &config;
}

bool IotConnectSdk_IsConnected() {
    return iotc_nrf_mqtt_is_connected();
}

///////////////////////////////////////////////////////////////////////////////////
// this the Initialization os IoTConnect SDK
int IotConnectSdk_Init() {
    discovery_response = run_http_discovery(config.cpid, config.env);
    if (NULL == discovery_response) {
        // get_base_url will print the error
        return -1;
    }

    char *sync_resp_str = run_http_sync(config.cpid, config.duid);
    if (NULL == sync_resp_str) {
        // Sync_call will print the error
        return -2;
    }

    sync_response = IOTCL_DiscoveryParseSyncResponse(sync_resp_str);
    if (!sync_response || sync_response->ds != IOTCL_SR_OK) {
        report_sync_error(sync_response, sync_resp_str);
        IOTCL_DiscoveryFreeSyncResponse(sync_response);
        sync_response = NULL;
        free(sync_resp_str);
        return -3;
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

    if (!IOTCL_Init(&lib_config)) {
        printk("Failed to initialize the IoTConnect Lib");
    }

    return 0;
}