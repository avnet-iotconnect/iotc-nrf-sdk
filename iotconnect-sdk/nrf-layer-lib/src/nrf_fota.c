/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <errno.h>
#include <net/fota_download.h>

#include "nrf_cert_store.h"
#include "nrf_fota.h"

static bool is_initialized = false;

// record the last fota config here
static IOTCONNECT_NRF_FOTA_CONFIG *fota_config = NULL;

static void nrf_fota_cb(const struct fota_download_evt *evt) {
    switch (evt->id) {
        case FOTA_DOWNLOAD_EVT_ERROR:
            printk("OTA: Received error from fota_download\n");
            break;
        case FOTA_DOWNLOAD_EVT_FINISHED:
            printk("OTA: Download finished\n");
            break;
        case FOTA_DOWNLOAD_EVT_PROGRESS:
            printk("OTA: Download progress: %d\n", evt->progress);
            break;
        case FOTA_DOWNLOAD_EVT_ERASE_PENDING:
            printk("OTA: Erase pending\n");
            break;
        case FOTA_DOWNLOAD_EVT_ERASE_DONE:
            printk("OTA: Erase done\n");
            break;
        default:
            break;
    }
    if (fota_config) {
        if (fota_config->fota_cb) {
            fota_config->fota_cb(evt);
        }
        if (evt->id == FOTA_DOWNLOAD_EVT_ERROR || evt->id == FOTA_DOWNLOAD_EVT_FINISHED) {
            fota_config = NULL;
        }
    }
}

int nrf_fota_init(void) {
    int retval = fota_download_init(nrf_fota_cb);
    if (retval) {
        printk("Error: Failed to fota_download_init!\n");
        return retval;
    }
    is_initialized = true;
    return 0;
}

/**@brief Start transfer of the file. */
int nrf_fota_start(IOTCONNECT_NRF_FOTA_CONFIG *config) {
    int retval;

    if (!is_initialized) {
        printk("nrf_ota_start: Must initialize the module first. Refusing to start now OTA!\n");
        return -EINVAL;
    }
    if (NULL != fota_config) {
        printk("nrf_ota_start: last ota OTA not complete. Refusing to start now OTA!\n");
        return -EBUSY;
    }

    if (NULL == config) {
        printk("nrf_ota_start: config argument is required for ota download\n");
        return -EINVAL;
    }
    if (NULL == config->host || NULL == config->path) {
        printk("nrf_ota_start: host and path are required for ota download\n");
        return -EINVAL;
    }
    int sec_tag = config->sec_tag;
    int port = config->port;


    if (port <= 0) {
        port = 443; // https
    }
    if (sec_tag == 0) {
        // use baltimore root cert for the download
        // set this to -1 for http, if outside of IoTConnect OTA
        sec_tag = TLS_SEC_TAG_IOTCONNECT_OTA;
    }

    /*
        For SDK 1.3.X replace the call below with this:
         retval = fota_download_start(
            config->host,
            config->path,
            sec_tag,
            port,
            config->apn // default APN (NULL)
        );
     */
    retval = fota_download_start(
            config->host,
            config->path,
            sec_tag,
            config->apn, /* default APN (NULL) */
            1024
    );
    if (0 == retval) {
        printk("Starting OTA download for host:%s path:%s, sec_tag:%d\n",
            config->host,
            config->path,
            sec_tag
        );
        fota_config = config;
    }
    return retval;
}

