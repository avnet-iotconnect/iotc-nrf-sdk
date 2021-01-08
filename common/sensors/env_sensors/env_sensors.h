/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ENV_SENSORS_H_
#define ENV_SENSORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file env_sensors.h
 * @defgroup env_sensors ENV_SENSORS Environmental sensors interface¨
 * @{
 * @brief Module for interfacing environmental sensors for asset tracker
 *
 * @details Basic basic for interfacing environmental sensors for the
 *          asset tracker application.
 *          Supported sensor types are Temperature, Humidity, Pressure and
 *          air quality sensors.
 *
 */

#include <zephyr/types.h>

typedef struct {
    /* light levels in lux */
    double temperature;
    double humidity;
    double pressure;
} env_sensor_data_t;


int env_sensors_init(void);
int env_sensors_get_data(env_sensor_data_t *data);


#ifdef __cplusplus
}
#endif

#endif /* ENV_SENSORS_H_ */
