/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTCONNECT_NRF_FOTA_H
#define IOTCONNECT_NRF_FOTA_H

#include <net/fota_download.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IOTCONNECT_NRF_OTA_CONFIG_T {
    const char *host;
    const char *path;
    int sec_tag; // optional sec tag to use for download. -1 for http. 0 is default: maps to TLS_SEC_TAG_MICROSOFT_CERTS
    int port; // optional for custom fota. default 0: maps to 433
    char *apn; // reserved for future use. Set it to NULL (0)
    fota_download_callback_t fota_cb; // optional user callback to handle NRF fota events. Default NULL (0)
} IOTCONNECT_NRF_FOTA_CONFIG;

/*
 * Example simple OTA initiation:
 *
 * ASSERT(nrf_fota_init());
 *
 * IOTCONNECT_NRF_FOTA_CONFIG fota_config = {0};
 * fota_config.host = "otahost.com";
 * fota_config.path = "ota/utrl/path";
 * nrf_fota_start(&ota_config);
 *
 */

// @brief Initializes the OTA module. Returns 0 on success.
int nrf_fota_init();

// @brief Starts FOTA. Returns 0 on success.
int nrf_fota_start(IOTCONNECT_NRF_FOTA_CONFIG *config);

#ifdef __cplusplus
}
#endif

#endif //IOTCONNECT_NRF_FOTA_H
