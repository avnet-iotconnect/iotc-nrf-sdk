/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file light_sensor.h
 * @defgroup light_sensors Light sensor interface
 * @{
 * @brief Module for interfacing light sensor for asset tracker
 *
 * @details Interface for RGB IR light sensor for asset tracker.
 *
 */
#include <zephyr.h>
#include <drivers/sensor.h>

typedef struct {
	/* light levels in lux */
	int32_t red;
	int32_t green;
	int32_t blue;
	int32_t ir;
} light_sensor_data_t;


int light_sensor_init(void);

int light_sensor_get_data(light_sensor_data_t * data);

#ifdef __cplusplus
}
#endif

#endif /* LIGHT_SENSOR_H_ */
