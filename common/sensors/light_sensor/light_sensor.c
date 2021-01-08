/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include "light_sensor.h"

#define DEV_NAME "BH1749"
static const struct device *ls_dev = NULL;

int light_sensor_init(void) {
    ls_dev = device_get_binding(DEV_NAME);
    if (ls_dev == NULL) {
        printk("Failed to initialize the light sensor\n");
        return -ENODEV;
    }
    return 0;
}

int light_sensor_get_data(light_sensor_data_t *data) {

    if (data == NULL) {
        return -EINVAL;
    }
    if (ls_dev == NULL) {
        return -ENODEV;
    }

    int err = sensor_sample_fetch_chan(ls_dev, SENSOR_CHAN_ALL);

    if (err) {
        printk("Failed to get data for light sensor. Error: %d\n ", err);
    }

    struct sensor_value sv = {0};
    err = sensor_channel_get(ls_dev, SENSOR_CHAN_RED, &sv); if (err) return err;
    data->red = sv.val1;
    err = sensor_channel_get(ls_dev, SENSOR_CHAN_GREEN, &sv); if (err) return err;
    data->green = sv.val1;
    err = sensor_channel_get(ls_dev, SENSOR_CHAN_BLUE, &sv); if (err) return err;
    data->blue = sv.val1;
    err = sensor_channel_get(ls_dev, SENSOR_CHAN_IR, &sv); if (err) return err;
    data->ir = sv.val1;

    return 0;
}
