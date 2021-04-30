/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTCONNECT_CERT_STORE_H
#define IOTCONNECT_CERT_STORE_H

#include <net/tls_credentials.h>
#include <net/mqtt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SEC_TAG where mqtt connection certs are stored on the modem */
#define TLS_SEC_TAG_IOTCONNECT_MQTT     10701

/* SEC_TAG where iotconnect API REST call certs are stored on the modem */
#define TLS_SEC_TAG_IOTCONNECT_API      10702

/* SEC_TAG where OTA download certs are stored on the modem */
#define TLS_SEC_TAG_IOTCONNECT_OTA      10703

// App interface

int nrf_cert_store_provision_api_certs(void);

int nrf_cert_store_provision_ota_certs(void);

int nrf_cert_store_save_device_cert(const char *device_private_key, const char *device_cert);

int nrf_cert_store_delete_all_device_certs();

int nrf_cert_store_configure_https_fd(sec_tag_t sec_tag, int fd);

// IoTConnect API Interface
int nrf_cert_store_configure_api_fd(int fd);

void nrf_cert_store_configure_tls(struct mqtt_sec_config *tls_config);

#ifdef __cplusplus
}
#endif
#endif // IOTCONNECT_CERT_STORE_H
