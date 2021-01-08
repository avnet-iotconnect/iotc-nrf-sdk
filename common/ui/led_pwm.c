/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <drivers/pwm.h>
#include <string.h>

#include "led_pwm.h"
#include "led_effect.h"

#define DEV_NAME "PWM_0"

#define LED_RED_PIN 29
#define LED_GREEN_PIN 30
#define LED_BLUE_PIN 31

const struct device *pwm_dev;

static const size_t led_pins[3] = {
	LED_RED_PIN,
	LED_GREEN_PIN,
	LED_BLUE_PIN,
};

static void pwm_out(struct led_color *color)
{
	for (size_t i = 0; i < ARRAY_SIZE(color->c); i++) {
		pwm_pin_set_usec(pwm_dev, led_pins[i],
				 255, color->c[i], 0);
	}
}

static void pwm_off()
{
	struct led_color nocolor = {0};

	pwm_out(&nocolor);
}

int ui_leds_init(void)
{
	const char *dev_name = DEV_NAME;
	int err = 0;

	pwm_dev = device_get_binding(dev_name);

	if (!pwm_dev) {
		printk("Could not bind to device %s\n", dev_name);
		return -ENODEV;
	}
    pwm_off();

	return err;
}

int ui_led_set_rgb(u8_t red, u8_t green, u8_t blue)
{
    if (!pwm_dev) {
        return -ENODEV;
    }
    struct led_color color;
	color.c[0] = red;
    color.c[1] = green;
    color.c[2] = blue;
	pwm_out(&color);
	return 0;
}
