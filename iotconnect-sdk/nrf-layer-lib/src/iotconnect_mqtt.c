//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by nmarkovi on 6/15/20.
//


#include <zephyr.h>
#include <stdio.h>
#include <string.h>

#include <net/mqtt.h>
#include <net/socket.h>
#include <modem/modem_key_mgmt.h>
#include <modem/lte_lc.h>

#include <cJSON.h>

#include "iotconnect_mqtt.h"
#include "iotconnect.h"
#include "nrf_cert_store.h"

#define MQTT_PUBACK_TIMEOUT_S       5 //5 secs
#define MAXIMUM_MSG_QUEUE           4
#define MAXIMUM_MSG_RESEND          1
typedef struct {
    bool in_used;
    uint32_t msg_id;
    const char *p_pub_topic;
    uint8_t *p_msg;
    uint32_t msg_len;
    uint32_t start_tick;
    uint8_t resend_count;
} msg_q_object_t;

static msg_q_object_t msg_queue[MAXIMUM_MSG_QUEUE];
SYS_MUTEX_DEFINE(mutex_msg_q);
static bool msg_queue_initialized = false;

extern struct mqtt_client client;
static IotconnectMqttConfig *config;
static unsigned int rolling_message_id = 1;
#if 1 //was_mod
static struct mqtt_utf8 mqtt_user_name;
#endif
static IotclSyncResponse *sync_response;
static uint8_t tx_buffer[MAXLINE];
static uint8_t rx_buffer[MAXLINE];
static uint8_t payload_buf[MAXLINE];
static struct pollfd fds;
static struct sockaddr_storage broker;


static bool connected = false;


static int subscribe(void);

static bool broker_address_init(void);

static bool client_init();

static int fds_init(struct mqtt_client *c);

static void mqtt_evt_handler(struct mqtt_client *const c, const struct mqtt_evt *evt);

static int nrf_mqtt_publish(struct mqtt_client *c, const char *topic, enum mqtt_qos qos,
                          const uint8_t *data, size_t len, uint32_t *p_msg_id);

static void msg_queue_clear(bool invoke_cb) {
    if (msg_queue_initialized) {
        //mutex lock
        sys_mutex_lock(&mutex_msg_q, K_FOREVER);
        for (int i=0; i<ARRAY_SIZE(msg_queue); i++) {
            if (msg_queue[i].in_used) {
                uint32_t msg_id = msg_queue[i].msg_id;
                free(msg_queue[i].p_msg);
                msg_queue[i].in_used = false;
                if (invoke_cb && config->msg_send_status_cb) {
                    config->msg_send_status_cb(msg_id, MSG_SEND_FAILED);
                }
            }
        }
        //printk("msg queue cleared\n");
        //mutex unlock
        sys_mutex_unlock(&mutex_msg_q);
    } 
}

static void msg_queue_init(void) {
    if (!msg_queue_initialized) {
        for (int i=0; i<ARRAY_SIZE(msg_queue); i++) {
            msg_queue[i].in_used = false;
        }
        msg_queue_initialized = true;
        //printk("msg queue initialized\n");
    }
    else {
        msg_queue_clear(false);
    }
}

static bool msg_queue_add(uint32_t msg_id, const char *p_pub_topic,
                            uint8_t *p_msg, uint32_t msg_len) {
    bool msg_added = false;
    if (msg_queue_initialized) {
        //mutex lock
        sys_mutex_lock(&mutex_msg_q, K_FOREVER);
        for (int i=0; i<ARRAY_SIZE(msg_queue); i++) {
            if (!msg_queue[i].in_used) {
                msg_queue[i].p_msg = (uint8_t *)malloc(msg_len);
                if (msg_queue[i].p_msg) {
                    memcpy(msg_queue[i].p_msg, p_msg, msg_len);
                    msg_queue[i].msg_len = msg_len;
                    msg_queue[i].msg_id = msg_id;
                    msg_queue[i].p_pub_topic = p_pub_topic;
                    msg_queue[i].start_tick = k_uptime_get_32();
                    msg_queue[i].resend_count = MAXIMUM_MSG_RESEND;
                    msg_queue[i].in_used = true;
                    msg_added = true;
                    //printk("message added to queue. (msg id: %d)\n", msg_id);
                }
                else {
                    printk("ERR: Unable to add message to queue!\n");
                }
                break;
            }
        }
        //mutex unlock
        sys_mutex_unlock(&mutex_msg_q);
    }
    return msg_added;
}

static void msg_queue_delete(uint32_t msg_id) {
    if (msg_queue_initialized) {
        //mutex lock
        sys_mutex_lock(&mutex_msg_q, K_FOREVER);
        for (int i=0; i<ARRAY_SIZE(msg_queue); i++) {
            if (msg_queue[i].in_used && (msg_queue[i].msg_id == msg_id)) {
                free(msg_queue[i].p_msg);
                msg_queue[i].in_used = false;
                //printk("message deleted from queue. (msg id: %d)\n", msg_id);
                break;
            }
        }
        //mutex unlock
        sys_mutex_unlock(&mutex_msg_q);
    }
}

static void msg_queue_proc(void) {
    if (msg_queue_initialized) {
        //mutex lock
        sys_mutex_lock(&mutex_msg_q, K_FOREVER);
        for (int i=0; i<ARRAY_SIZE(msg_queue); i++) {
            if (msg_queue[i].in_used) {
                if ((k_uptime_get_32() - msg_queue[i].start_tick) >= (MQTT_PUBACK_TIMEOUT_S * 1000)) {
                    //printk("ERR: Send message timeout! (msg id: %d)\n", msg_queue[i].msg_id);
                    if (msg_queue[i].resend_count) {
                        printk("Re-send message again. (msg id: %d)\n", msg_queue[i].msg_id);
                        uint32_t msg_id = msg_queue[i].msg_id;
                        int err = nrf_mqtt_publish(&client, msg_queue[i].p_pub_topic, MQTT_QOS_1_AT_LEAST_ONCE,
                                    msg_queue[i].p_msg, msg_queue[i].msg_len, &msg_id);
                        if (err) {
                            printk("message resend failed! %d\n", err);
                        }
                        msg_queue[i].start_tick = k_uptime_get_32();
                        msg_queue[i].resend_count--;
                    }
                    else {
                        //printk("Maximum re-send reached! Abort message! (msg id: %d)\n", msg_queue[i].msg_id);
                        uint32_t msg_id = msg_queue[i].msg_id;
                        msg_queue_delete(msg_id);
                        if (config->msg_send_status_cb) {
                            config->msg_send_status_cb(msg_id, MSG_SEND_TIMEOUT);
                        }
                    }
                }
            }
        }
        //mutex unlock
        sys_mutex_unlock(&mutex_msg_q);
    }
}

static unsigned int get_next_message_id() {
    if (rolling_message_id + 1 >= UINT_MAX) {
        rolling_message_id = 1;
    }
    return rolling_message_id++;
}

/**@brief Function to read the published payload.
 */
static int get_event_payload(struct mqtt_client *c, size_t length) {
    uint8_t *buf = payload_buf;
    uint8_t *end = buf + length;

    if (length > sizeof(payload_buf)) {
        return -EMSGSIZE;
    }

    while (buf < end) {
        int ret = mqtt_read_publish_payload(c, buf, end - buf);

        if (ret < 0) {
            int err;

            if (ret != -EAGAIN) {
                return ret;
            }

            printk("get_event_payload: EAGAIN\n");

            err = poll(&fds, 1, 1000 * CONFIG_MQTT_KEEPALIVE);
            if (err > 0 && (fds.revents & POLLIN) == POLLIN) {
                continue;
            } else {
                return -EIO;
            }
        }

        if (ret == 0) {
            return -EIO;
        }

        buf += ret;
    }

    return 0;
}

static bool broker_address_init(void) {
    int err;
    struct addrinfo *result;
    struct addrinfo *addr;
    struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM
    };

    err = getaddrinfo(sync_response->broker.host, NULL, &hints, &result);

    if (err) {
        printk("ERROR: getaddrinfo failed %d\n", err);
        return false;
    }

    addr = result;
    err = (-ENOENT);


    while (addr != NULL) {
        if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
            struct sockaddr_in *broker4 =
                    ((struct sockaddr_in *) &broker);
            char ipv4_addr[NET_IPV4_ADDR_LEN];

            broker4->sin_addr.s_addr =
                    ((struct sockaddr_in *) addr->ai_addr)
                            ->sin_addr.s_addr;
            broker4->sin_family = AF_INET;
            broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);

            inet_ntop(AF_INET, &broker4->sin_addr.s_addr,
                      ipv4_addr, sizeof(ipv4_addr));
            printk("MQTT IPv4 Address: %s\n", ipv4_addr);

            freeaddrinfo(result);

            return true;
        } else {
            printk("Need IPV4 address. Length is %u, should be %u\n",
                   (unsigned int) addr->ai_addrlen,
                   (unsigned int) sizeof(struct sockaddr_in));
        }
        // TODO: Deal with multiple addreses. What about cname? Will this solve it?
        addr = addr->ai_next;
    }

    freeaddrinfo(result);

    return false;
}

static int fds_init(struct mqtt_client *c) {
    if (c->transport.type == MQTT_TRANSPORT_NON_SECURE) {
        fds.fd = c->transport.tcp.sock;
    } else {
#if IS_ENABLED(CONFIG_MQTT_LIB_TLS)
        fds.fd = c->transport.tls.sock;
#else
        return -ENOTSUP;
#endif
    }

    fds.events = POLLIN;

    return 0;
}

static bool client_init() {
    mqtt_client_init(&client);

    if (!broker_address_init()) {
        printk("broker_address_init failed!\n");
        return false;
    }

    client.broker = &broker;
    client.evt_cb = mqtt_evt_handler;

#if 1 //wads_mod

    client.client_id.utf8 = sync_response->broker.client_id;
    client.client_id.size = strlen(sync_response->broker.client_id);

    mqtt_user_name.utf8 = sync_response->broker.user_name;
    mqtt_user_name.size = strlen(sync_response->broker.user_name);

    client.user_name = &mqtt_user_name;
    client.password = NULL;
#else
    client.client_id.utf8 = (uint8_t *)CONFIG_MQTT_CLIENT_ID;
    client.client_id.size = strlen(CONFIG_MQTT_CLIENT_ID);
    client.password = NULL;
    client.user_name = NULL;
#endif //wads_mod

    client.protocol_version = MQTT_VERSION_3_1_1;

    client.rx_buf = rx_buffer;
    client.rx_buf_size = sizeof(rx_buffer);
    client.tx_buf = tx_buffer;
    client.tx_buf_size = sizeof(tx_buffer);

#if IS_ENABLED(CONFIG_MQTT_LIB_TLS)
    struct mqtt_sec_config *tls_config = &client.transport.tls.config;

    client.transport.type = MQTT_TRANSPORT_SECURE;
    tls_config->peer_verify = config->tls_verify;

    tls_config->cipher_count = 0;
    tls_config->cipher_list = NULL;
    nrf_cert_store_configure_tls(tls_config);
    tls_config->hostname = sync_response->broker.host;

#else
    client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif
    return true;
}

static int nrf_mqtt_publish(struct mqtt_client *c, const char *topic, enum mqtt_qos qos,
                          const uint8_t *data, size_t len, uint32_t *p_msg_id) {
    struct mqtt_publish_param param;
    int code;

    param.message.topic.qos = qos;
    param.message.topic.topic.utf8 = (uint8_t *) topic;
    param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);
    param.message.payload.data = (uint8_t *) data;
    param.message.payload.len = len;
    // if p_msg_id contains non-zero value, the message is re-send from msg_queue_proc().
    // Assign this message with assigned messsage id in p_msg_id.
    // else assign current running message id to this message.
    if (p_msg_id && *p_msg_id) {
        param.message_id = *p_msg_id;
    }
    else {
        param.message_id = get_next_message_id();
    }
    param.dup_flag = 0;
    param.retain_flag = 0;

    code = mqtt_publish(c, &param);

    // Only add the message to msg queue when:
    //   mqtt_publish() is successful.
    //   p_msg_id is NULL or contains zero. (not re-send from msg_queue_proc())
    //  QOS is 1.
    if (!code && !(p_msg_id && *p_msg_id) && (qos == MQTT_QOS_1_AT_LEAST_ONCE)) {
        msg_queue_add(param.message_id, topic, (uint8_t *)data, len);
        // Return back the assigned message id if p_msg_id is not NULL.
        if (p_msg_id) {
            *p_msg_id = param.message_id;
        }
    }

    return code;
}

/**@brief Function to publish data on the configured topic
 */
int iotc_nrf_mqtt_publish(struct mqtt_client *c, const char *topic, enum mqtt_qos qos,
                          const uint8_t *data, size_t len, uint32_t *p_msg_id) {

    if (p_msg_id ) {
        //New message. Force p_msg_id to zero.
        *p_msg_id = 0;
    }
    return nrf_mqtt_publish(c, topic, qos, data, len, p_msg_id);
}

static void mqtt_evt_handler(struct mqtt_client *const c,
                             const struct mqtt_evt *evt) {
    int err;
    switch (evt->type) {
        case MQTT_EVT_CONNACK:
            if (evt->result != 0) {
                connected = false;
                printk("MQTT connect failed %d\n", evt->result);
                if (config->status_cb) {
                    config->status_cb(MQTT_FAILED);
                }
                break;
            }

            connected = true;
            printk("MQTT client connected!\n");
            subscribe();
            if (config->status_cb) {
                config->status_cb(MQTT_CONNECTED);
            }
            break;

        case MQTT_EVT_DISCONNECT:
            printk("MQTT client disconnected %d\n", evt->result);
            fds.fd = -1;
            connected = false;
            if (config->status_cb) {
                config->status_cb(MQTT_DISCONNECTED);
            }
            msg_queue_clear(true);
            break;

        case MQTT_EVT_PUBLISH: {
            const struct mqtt_publish_param *p = &evt->param.publish;
            // mqtt_publish_qos1_ack(c, &evt->param.puback); // this doesn't seem to work with QOS_1
            printk("MQTT PUBLISH result=%d len=%d\n", evt->result, p->message.payload.len);
            err = get_event_payload(c, p->message.payload.len);
            if (err >= 0) {

                if (config->data_cb) {
                    config->data_cb(payload_buf, p->message.payload.len, p->message.topic.topic.utf8);
                } else {
                    printk("Error: no mqtt data_cb configured\n");
                }

            } else {
                printk("mqtt_read_publish_payload: Failed! %d\n", err);
                printk("Disconnecting MQTT client...\n");

                err = mqtt_disconnect(c);
                if (err) {
                    printk("Could not disconnect: %d\n", err);
                }
            }
        }
            break;

        case MQTT_EVT_PUBACK:
            if (evt->result != 0) {
                printk("MQTT PUBACK error %d\n", evt->result);
                break;
            }

            printk("Packet id: %u acknowledged\n", evt->param.puback.message_id);
            msg_queue_delete(evt->param.puback.message_id);
            if (config->msg_send_status_cb) {
                config->msg_send_status_cb(evt->param.puback.message_id, MSG_SEND_SUCCESS);
            }
            break;

        case MQTT_EVT_SUBACK:
            if (evt->result != 0) {
                printk("MQTT SUBACK error %d\n", evt->result);
                break;
            }

            printk("SUBACK packet id: %u\n", evt->param.suback.message_id);
            break;

        default:
            printk("default: %d\n", evt->type);
            break;
    }
}

static int subscribe(void) {
    struct mqtt_topic subscribe_topics[] = {
            {.topic = {
                    .utf8 = sync_response->broker.sub_topic,
                    .size = strlen(sync_response->broker.sub_topic)
            },
                    .qos = MQTT_QOS_0_AT_MOST_ONCE
            }/*
            ,

            {.topic = {
                    .utf8 = twinPropertySubTopic,
                    .size = strlen(twinPropertySubTopic)
            },
                    .qos = MQTT_QOS_0_AT_MOST_ONCE
            },
            {.topic = {
                    .utf8 = twinResponseSubTopic,
                    .size = strlen(twinResponseSubTopic)
            },
                    .qos = MQTT_QOS_0_AT_MOST_ONCE
            }
            */
    };

    const struct mqtt_subscription_list subscription_list = {
            .list = &subscribe_topics[0],
            .list_count = ARRAY_SIZE(subscribe_topics),
            .message_id = get_next_message_id()
    };


    return mqtt_subscribe(&client, &subscription_list);
}

bool iotc_nrf_mqtt_is_connected() {
    return connected;
}


///////////////////////////////////////////////////////////////////////////////////
// Start the MQTT protocol
bool iotc_nrf_mqtt_init(IotconnectMqttConfig *c, IotclSyncResponse *sr) {

    int err;

    if (!c) {
        return false;
    }

    if (!sr || IOTCL_SR_OK != sr->ds) {
        iotcl_discovery_free_sync_response(sr);
        return false;
    }

    //init msg queue
    msg_queue_init();

    fds.fd = -1;
    sync_response = sr;
    config = c;

    if (!client_init()) {
        return false;
    }

    err = mqtt_connect(&client);
    if (err != 0) {
        printk("ERROR: mqtt_connect %d\n", err);
        return false;
    }

    err = fds_init(&client);
    if (err != 0) {
        printk("ERROR: fds_init %d\n", err);
        return false;
    }

    return true;

}

///////////////////////////////////////////////////////////////////////////////////
// MQTT will work in while loop
void iotc_nrf_mqtt_loop(void) {
    int err;

    // process msg queue procedure
    msg_queue_proc();

    //if no FD return.
    if (fds.fd == -1) {
        return;
    }

    err = poll(&fds, 1, 10);
    if (err < 0) {
        printk("ERROR: poll %d\n", errno);
        return;
    }

    if (mqtt_keepalive_time_left(&client) <= 0 && iotc_nrf_mqtt_is_connected()) {
        printf("Sending MQTT Keepalive\n");
        err = mqtt_live(&client);
        if ((err != 0) && (err != -EAGAIN)) {
            printk("ERROR: mqtt_live %d\n", err);
            return;
        }
    }

    if ((fds.revents & POLLIN) == POLLIN) {
        err = mqtt_input(&client);
        if (err != 0) {
            printk(">> ERROR: mqtt_input %d\n", err);
            return;
        }
    }

    if ((fds.revents & POLLERR) == POLLERR) {
        printk("POLLERR\n");
        return;
    }

    if ((fds.revents & POLLNVAL) == POLLNVAL) {
        printk("POLLNVAL\n");
        return;
    }
}

void iotc_nrf_mqtt_abort() {

    mqtt_abort(&client);
}
