/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <drivers/sensor.h>
#include "env_sensors.h"

#define DEV_NAME "BME680"

static const struct device *env_dev = NULL;

/**@brief Initialize environment sensors. */
int env_sensors_init(void)
{
    env_dev = device_get_binding(DEV_NAME);
    if (!env_dev) {
        printk("Unable to initialize environmental sensors");
        return -ENODEV;
    }
	return 0;
}

int env_sensors_get_data(env_sensor_data_t *data) {

    if (data == NULL) {
        return -EINVAL;
    }
    if (env_dev == NULL) {
        return -ENODEV;
    }

    int err = sensor_sample_fetch_chan(env_dev, SENSOR_CHAN_ALL);

    if (err) {
        printk("Failed to get data for environment sensors. Error: %d\n ", err);
    }

    struct sensor_value sv = {0};
    err = sensor_channel_get(env_dev, SENSOR_CHAN_AMBIENT_TEMP, &sv); if (err) return err;
    data->temperature = sensor_value_to_double(&sv);
    err = sensor_channel_get(env_dev, SENSOR_CHAN_HUMIDITY, &sv); if (err) return err;
    data->humidity = sensor_value_to_double(&sv);
    err = sensor_channel_get(env_dev, SENSOR_CHAN_PRESS, &sv); if (err) return err;
    data->pressure = sensor_value_to_double(&sv);

    return 0;
}




