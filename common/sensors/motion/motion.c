/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <drivers/sensor.h>
#include "motion.h"


static const struct device *motion_dev = NULL;

#define FLIP_ACCELERATION_THRESHOLD    5.0

static int get_orientation(motion_orientation_state_t *orientation,
                           motion_acceleration_data_t *acceleration_data) {
    if (orientation == NULL || acceleration_data == NULL) {
        return -EINVAL;
    }

    if (acceleration_data->z >= FLIP_ACCELERATION_THRESHOLD) {
        *orientation = IS_ENABLED(CONFIG_ACCEL_INVERTED) ?
                       MOTION_ORIENTATION_UPSIDE_DOWN : MOTION_ORIENTATION_NORMAL;
    } else if (acceleration_data->z <= -FLIP_ACCELERATION_THRESHOLD) {
        *orientation = IS_ENABLED(CONFIG_ACCEL_INVERTED) ?
                       MOTION_ORIENTATION_NORMAL : MOTION_ORIENTATION_UPSIDE_DOWN;
    } else {
        *orientation = MOTION_ORIENTATION_ON_SIDE;
    }

    return 0;
}


/**@brief Workqueue handler that runs the callback provided by application.*/
int motiondata_to_orientation(motion_data_t *motion_data) {
    int err = get_orientation(&motion_data->orientation, &motion_data->acceleration);
    if (err) {
        return err;
    }
    return 0;
}


#define DEV_NAME "ADXL362"

/**@brief Initialize environment sensors. */
int accelerometer_init(void) {
    motion_dev = device_get_binding(DEV_NAME);
    if (!motion_dev) {
        printk("Unable to initialize the accelerometer");
        return -ENODEV;
    }
    return 0;
}

int accelerometer_get_data(motion_data_t *data) {

    if (data == NULL) {
        return -EINVAL;
    }
    if (motion_dev == NULL) {
        return -ENODEV;
    }

    int err = sensor_sample_fetch_chan(motion_dev, SENSOR_CHAN_ALL);
    if (err) {
        printk("Failed to get data for accelerometer. Error: %d\n ", err);
    }

    struct sensor_value sv = {0};
    err = sensor_channel_get(motion_dev, SENSOR_CHAN_ACCEL_X, &sv);
    if (err) return err;
    data->acceleration.x = sensor_value_to_double(&sv);
    err = sensor_channel_get(motion_dev, SENSOR_CHAN_ACCEL_Y, &sv);
    if (err) return err;
    data->acceleration.y = sensor_value_to_double(&sv);
    err = sensor_channel_get(motion_dev, SENSOR_CHAN_ACCEL_Z, &sv);
    if (err) return err;
    data->acceleration.z = sensor_value_to_double(&sv);

    motiondata_to_orientation(data);

    return 0;
}

