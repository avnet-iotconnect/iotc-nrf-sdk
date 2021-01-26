/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

//
// Copyright: Avnet 2021
// Created by Shu Liu <shu.liu@avnet.com>
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 1/25/21. - Add AGPS support
//

#include <zephyr.h>
#include <sys/util.h>
#include <drivers/gps.h>
#include <modem/lte_lc.h>
#include <string.h>

#include "gps_controller.h"

#define DEV_NAME "NRF9160_GPS"

#define GPS_PROCESS_STACK_SIZE  KB(4)
#define GPS_PROCESS_PRIORITY    K_LOWEST_APPLICATION_THREAD_PRIO

K_THREAD_STACK_DEFINE(gps_process_stack_area, GPS_PROCESS_STACK_SIZE);

K_THREAD_STACK_DEFINE(gps_process_stack_area,
                      GPS_PROCESS_STACK_SIZE);

static struct k_work_q gps_work_q;
static struct k_delayed_work send_agps_request_work;
static struct gps_agps_request agps_request;
static const struct device *gps_dev;
static enum lte_lc_system_mode online_system_mode;

static void send_agps_request(struct k_work *work) {
    ARG_UNUSED(work);

//#if defined(CONFIG_AGPS)
#if defined(KB)
    int err;
    static int64_t last_request_timestamp;

/* Request A-GPS data no more often than every hour (time in milliseconds). */
#define AGPS_UPDATE_PERIOD (60 * 60 * 1000)

    if ((last_request_timestamp != 0) &&
        (k_uptime_get() - last_request_timestamp) < AGPS_UPDATE_PERIOD) {
        printk("A-GPS request was sent less than 1 hour ago\n");
        return;
    }

    printk("Sending A-GPS request\n");
    /*
    lte_lc_offline();
    lte_lc_system_mode_set(online_system_mode);
    lte_lc_normal();
    */
    err = gps_agps_request(agps_request, GPS_SOCKET_NOT_PROVIDED);
    /*
    if (err) {
        printk("Failed to request A-GPS data, error: %d\n", err);
        return;
    }
    lte_lc_offline();
    lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_GPS);
    lte_lc_normal();
  */
    last_request_timestamp = k_uptime_get();

    printk("A-GPS request sent\n");
#endif /* defined(CONFIG_AGPS) */
}

void submit_agps_request(struct gps_agps_request *agps_request_ptr, enum lte_lc_system_mode online_system_mode_) {
    memcpy(&agps_request, agps_request_ptr, sizeof(struct gps_agps_request));
    online_system_mode = LTE_LC_SYSTEM_MODE_LTEM_GPS;
    switch (online_system_mode_) {
        case LTE_LC_SYSTEM_MODE_NONE:
        case LTE_LC_SYSTEM_MODE_GPS:
            printk("No valid online system mode set.. Assuming LTE+GPS...\n");
            online_system_mode = LTE_LC_SYSTEM_MODE_LTEM_GPS;
            break;
        case LTE_LC_SYSTEM_MODE_LTEM:
        case LTE_LC_SYSTEM_MODE_LTEM_GPS:
            online_system_mode = LTE_LC_SYSTEM_MODE_LTEM_GPS;
            break;
        case LTE_LC_SYSTEM_MODE_NBIOT:
        case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
            online_system_mode = LTE_LC_SYSTEM_MODE_NBIOT_GPS;
            break;
        default:
            printk("Invalid system mode requested");
            return;
    }

    (void)online_system_mode; // avoid warnings in case we disable mode switching

    online_system_mode = online_system_mode_;
    k_delayed_work_submit_to_queue(&gps_work_q,
                                   &send_agps_request_work,
                                   K_SECONDS(1));

}

static void start(void) {
    int err;
    struct gps_config gps_cfg = {
            .nav_mode = GPS_NAV_MODE_CONTINUOUS,
            .power_mode = GPS_POWER_MODE_DISABLED,
            .timeout = 0,
            .interval = 0
    };

    if (gps_dev == NULL) {
        printk("GPS controller is not initialized properly\n");
        return;
    }

    err = gps_start(gps_dev, &gps_cfg);
    if (err) {
        printk("Failed to enable GPS, error: %d\n", err);
        return;
    }

    printk("GPS started successfully. Searching for satellites...\n");
}

static void stop(void) {
    int err;

    if (gps_dev == NULL) {
        printk("GPS controller is not initialized\n");
        return;
    }

    err = gps_stop(gps_dev);
    if (err) {
        printk("Failed to disable GPS, error: %d\n", err);
        return;
    }

    printk("GPS operation was stopped\n");
}

void gps_control_start(void) {
    start();
}

void gps_control_stop(void) {
    stop();
}

/** @brief Configures and starts the GPS device. */
int gps_control_init(gps_event_handler_t handler) {
    int err;
    static bool is_init;

    if (is_init) {
        return -EALREADY;
    }

    gps_dev = device_get_binding(DEV_NAME);
    if (gps_dev == NULL) {
        printk("Could not get %s device\n", DEV_NAME);
        return -ENODEV;
    }

    err = gps_init(gps_dev, handler);
    if (err) {
        printk("Could not initialize GPS, error: %d\n", err);
        return err;
    }
    k_work_q_start(&gps_work_q, gps_process_stack_area,
                   K_THREAD_STACK_SIZEOF(gps_process_stack_area),
                   GPS_PROCESS_PRIORITY);

    k_delayed_work_init(&send_agps_request_work, send_agps_request);

    printk("GPS initialized\n");

    is_init = true;

    return err;
}
