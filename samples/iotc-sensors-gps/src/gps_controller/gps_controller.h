/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 *
 * @brief   GPS module for asset tracker
 */

#ifndef GPS_CONTROLLER_H__
#define GPS_CONTROLLER_H__

#include <zephyr.h>
#include <modem/lte_lc.h>
#include <drivers/gps.h>

#ifdef __cplusplus
extern "C" {
#endif

int gps_control_init(gps_event_handler_t handler);

void gps_control_start(void);

void submit_agps_request(struct gps_agps_request *agps_request_ptr, enum lte_lc_system_mode online_system_mode_);

void gps_control_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* GPS_CONTROLLER_H__ */
