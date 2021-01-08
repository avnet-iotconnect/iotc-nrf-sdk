/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTCONNECT_CERT_STORE_H
#define IOTCONNECT_CERT_STORE_H

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
int NrfCertStore_ProvisionApiCerts(void);

int NrfCertStore_ProvisionOtaCerts(void);

int NrfCertStore_SaveDeviceCert(const char *device_private_key, const char *device_cert);

int NrfCertStore_DeleteAllDeviceCerts();

// IoTConnect Interface
int NrfCertStore_ConfigureApiFd(int fd);

void NrfCertStore_ConfigureTls(struct mqtt_sec_config *tls_config);

#ifdef __cplusplus
}
#endif
#endif // IOTCONNECT_CERT_STORE_H
