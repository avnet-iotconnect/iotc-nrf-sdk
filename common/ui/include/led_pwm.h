/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 *
 * @brief   LED control for the User Interface module. The module uses PWM to
 *	    control RGB colors and light intensity.
 */

#ifndef UI_LEDS_H__
#define UI_LEDS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initializes LEDs in the user interface module. */
int ui_leds_init(void);

/**@brief Sets RGB and light intensity values, in 0 - 255 ranges. */
int ui_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

#ifdef __cplusplus
}
#endif

#endif /* UI_LEDS_H__ */
