
#include <string.h>
#include <zephyr.h>
#include <net/socket.h>
#include <modem/modem_key_mgmt.h>

#include "iotconnect_certs_cloud_svcs.h"
#include "nrf_cert_store.h"


#if IS_ENABLED(CONFIG_MQTT_LIB_TLS)
static sec_tag_t sec_tag_list[] = { TLS_SEC_TAG_IOTCONNECT_MQTT };
#endif

void nrf_cert_store_configure_tls(struct mqtt_sec_config *tls_config) {
    tls_config->sec_tag_count = ARRAY_SIZE(sec_tag_list);
    tls_config->sec_tag_list = sec_tag_list;
}

///////////////////////////////////////////////////////////////////////////////////
/* Setup TLS options on a given socket */


///////////////////////////////////////////////////////////////////////////////////
/* Setup TLS options on a given socket */
int nrf_cert_store_configure_https_fd(sec_tag_t sec_tag, int fd) {
    int err;
    int verify;

    const sec_tag_t tls_sec_tag[] = {
            sec_tag,
    };

    verify = CONFIG_PEER_VERIFY;

    err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
    if (err) {
        printk("Failed to setup peer verification, err %d\n", errno);
        return err;
    }

    err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag,
                     sizeof(tls_sec_tag));
    if (err) {
        printk("Failed to setup TLS sec tag, err %d\n", errno);
    }

    return err;
}


int nrf_cert_store_configure_api_fd(int fd) {
    return nrf_cert_store_configure_https_fd(TLS_SEC_TAG_IOTCONNECT_API, fd);
}

static int provision_ca_cert_if_no_exists(int sec_tag, char *cert) {
    int err;

    bool exists = false;
    uint8_t dummy_perm_flags;
    err = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists, &dummy_perm_flags);
    if (err) {
        printk("modem_key_mgmt_exists error %d\n", err);
        return err;
    }

    if (exists) {
        printk("modem_key_mgmt: sec tag %d already stored on the device\n", sec_tag);
        return 0;
    }

    /* Delete certificates up to 5 certs from the modem storage for our sec key
 * in case there are any other remaining */
    for (int index = 0; index < 5; index++) {
        err = modem_key_mgmt_delete(sec_tag, index);
        printk("modem_key_mgmt_delete(%d, %d) = %d\n",
               sec_tag, index, err);
    }

    printk("Provisioning sec tag %d certificate\n", sec_tag);
    /*  Provision certificate to the modem */
    err = modem_key_mgmt_write(sec_tag,
                               MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                               cert, strlen(cert));
    if (err) {
        printk("Failed to provision certificate, err %d\n", err);
        return err;
    }
    return 0;
}

/* Provision certificate to modem */
int nrf_cert_store_provision_api_certs() {
    return provision_ca_cert_if_no_exists(TLS_SEC_TAG_IOTCONNECT_API, CERT_GODADDY_INT_SECURE_G2);
}

int nrf_cert_store_provision_ota_certs() {
    return provision_ca_cert_if_no_exists(TLS_SEC_TAG_IOTCONNECT_OTA, CERT_BALTIMORE_ROOT_CA);
}

int nrf_cert_store_save_device_cert(const char *device_private_key, const char *device_cert) {
    int err = 0;

    const char *certificates[] = {CERT_BALTIMORE_ROOT_CA,
                                  device_private_key,
                                  device_cert
    };
    //nrf_sec_tag_t sec_tag = TLS_SEC_TAG_IOTCONNECT_MQTT; //CONFIG_CLOUD_CERT_SEC_TAG;
    enum modem_key_mgnt_cred_type credentials[] = {
            MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
            MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
            MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
    };

    /* Delete certificates up to 5 certs from the modem storage for our sec key
     * in case there are any other remaining */
    for (int index = 0; index < 5; index++) {
        (void) modem_key_mgmt_delete(TLS_SEC_TAG_IOTCONNECT_MQTT, index);
        printk("modem_key_mgmt_delete(%d, %d) => result=%d\n",
               TLS_SEC_TAG_IOTCONNECT_MQTT, index, err);
    }

    /* Write certificates */
    for (enum modem_key_mgnt_cred_type type = 0; type < ARRAY_SIZE(credentials); type++) {
        err |= modem_key_mgmt_write(TLS_SEC_TAG_IOTCONNECT_MQTT, credentials[type],
                                    certificates[type], strlen(certificates[type]));
        printk("modem_key_mgmt_write => result=%d\n", err);
    }
    return err;
}

int nrf_cert_store_delete_all_device_certs() {
    int err = 0;

    /* Delete certificates up to 5 certs from the modem storage for our sec key
     * in case there are any other remaining */
    for (int index = 0; index < 5; index++) {
        err |= modem_key_mgmt_delete(TLS_SEC_TAG_IOTCONNECT_MQTT, index);
        printk("modem_key_mgmt_delete(%d, %d) => result=%d\n",
               TLS_SEC_TAG_IOTCONNECT_MQTT, index, err);
    }

    return 0;
}
