/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <drivers/sensor.h>
#include "env_sensors.h"

#if IS_ENABLED(CONFIG_BOARD_THINGY91_NRF9160NS)
#define DEV_NAME            "BME680"
#elif IS_ENABLED(CONFIG_BOARD_NRF9160_AVT9152NS)
#define DEV_NAME            "LPS22HB"
#define HUMIDITY_DEV_NAME   "TE23142771"
#else
#define DEV_NAME            CONFIG_SENSOR_SIM_DEV_NAME
#endif

static const struct device *env_dev = NULL;
#if IS_ENABLED(CONFIG_BOARD_NRF9160_AVT9152NS)
static const struct device *humidity_dev = NULL;
#endif

/**@brief Initialize environment sensors. */
int env_sensors_init(void)
{
    env_dev = device_get_binding(DEV_NAME);
    if (!env_dev) {
        printk("Unable to initialize environmental sensors\n");
        return -ENODEV;
    }
    printk("environmental sensors: %s\n", DEV_NAME);
#if IS_ENABLED(CONFIG_BOARD_NRF9160_AVT9152NS)
    humidity_dev = device_get_binding(HUMIDITY_DEV_NAME);
    if (!humidity_dev) {
        printk("Unable to initialize humidity sensor\n");
        return -ENODEV;
    }
    printk("humidity sensor: %s\n", HUMIDITY_DEV_NAME);
#endif

    return 0;
}

int env_sensors_get_data(env_sensor_data_t *data) {

    if (data == NULL) {
        return -EINVAL;
    }
    if (env_dev == NULL) {
        return -ENODEV;
    }
#if IS_ENABLED(CONFIG_BOARD_NRF9160_AVT9152NS)
    if (humidity_dev == NULL) {
        return -ENODEV;
    }
#endif

    int err;
#if IS_ENABLED(CONFIG_BOARD_THINGY91_NRF9160NS) || IS_ENABLED(CONFIG_BOARD_NRF9160_AVT9152NS)
    err = sensor_sample_fetch_chan(env_dev, SENSOR_CHAN_ALL);
    if (err) {
        printk("Failed to get data from %s. Error: %d\n ", DEV_NAME, err);
        return err;
    }

#if IS_ENABLED(CONFIG_BOARD_NRF9160_AVT9152NS)
    err = sensor_sample_fetch_chan(humidity_dev, SENSOR_CHAN_HUMIDITY);
    if (err) {
        printk("Failed to get humidity from %s Error: %d\n ", HUMIDITY_DEV_NAME, err);
        return err;
    }
#endif
#else
    err = sensor_sample_fetch_chan(env_dev, SENSOR_CHAN_AMBIENT_TEMP);
    if (err) {
        printk("Failed to get temperature from sensor_sim. Error: %d\n ", err);
        return err;
    }
    err = sensor_sample_fetch_chan(env_dev, SENSOR_CHAN_HUMIDITY);
    if (err) {
        printk("Failed to get humidity from sensor_sim. Error: %d\n ", err);
        return err;
    }
    err = sensor_sample_fetch_chan(env_dev, SENSOR_CHAN_PRESS);
    if (err) {
        printk("Failed to get pressure from sensor_sim. Error: %d\n ", err);
        return err;
    }
#endif

    struct sensor_value sv = {0};
    err = sensor_channel_get(env_dev, SENSOR_CHAN_AMBIENT_TEMP, &sv); if (err) return err;
    data->temperature = sensor_value_to_double(&sv);
#if IS_ENABLED(CONFIG_BOARD_NRF9160_AVT9152NS)
    err = sensor_channel_get(humidity_dev, SENSOR_CHAN_HUMIDITY, &sv); if (err) return err;
#else
    err = sensor_channel_get(env_dev, SENSOR_CHAN_HUMIDITY, &sv); if (err) return err;
#endif
    data->humidity = sensor_value_to_double(&sv);
    err = sensor_channel_get(env_dev, SENSOR_CHAN_PRESS, &sv); if (err) return err;
    data->pressure = sensor_value_to_double(&sv);

    return 0;
}




