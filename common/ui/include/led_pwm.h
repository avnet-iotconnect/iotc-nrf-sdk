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

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initializes LEDs in the user interface module. */
int ui_leds_init(void);

/**@brief Sets RGB and light intensity values, in 0 - 255 ranges. */
int ui_led_set_rgb(u8_t red, u8_t green, u8_t blue);

#ifdef __cplusplus
}
#endif

#endif /* UI_LEDS_H__ */
