/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/util.h>
#include <drivers/gps.h>
#include <modem/lte_lc.h>

#include "gps_controller.h"

#define DEV_NAME "NRF9160_GPS"

/* Structure to hold GPS work information */
static const struct device *gps_dev;

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

    printk("GPS initialized\n");

    is_init = true;

    return err;
}
