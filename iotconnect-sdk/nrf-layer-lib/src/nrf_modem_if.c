//
// Copyright: Avnet, Softweb Inc. 2020
// Created by nmarkovi on 6/15/20.
//

#include <zephyr.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>
#include <net/socket.h>
#include <kernel.h>
#include <date_time.h>
#include <modem/modem_info.h>
#include "nrf_modem_if.h"

#define DATE_TIME_TIMEOUT_S 15
static time_t rtc_offset_secs = 0;

// Modem params:
static bool has_modem_params = false;
static struct modem_param_info modem_params;

static K_SEM_DEFINE(date_time_obtained, 0, 1);

int _gettimeofday(struct timeval *tv, void *tzvp) {
    // if either no time, or someone is trying to use timezone offset, we cannnot support
    if (0 == rtc_offset_secs) {
        return -1;
    }
    u32_t rtc_now_secs = k_uptime_get_32() / 1000;
    time_t time_now = rtc_offset_secs + rtc_now_secs;
    tv->tv_sec = time_now;
    tv->tv_usec = 0;
    //printk("kt: %u, tod: %lu\n", rtc_now, t_of_day);
    return 0;  // return non-zero for error
} // end _gettimeofday()

static void date_time_event_handler(const struct date_time_evt *evt)
{
    switch (evt->type) {
        case DATE_TIME_OBTAINED_MODEM:
            printk("DATE_TIME_OBTAINED_MODEM\n");
            break;
        case DATE_TIME_OBTAINED_NTP:
            printk("DATE_TIME_OBTAINED_NTP\n");
            break;
        case DATE_TIME_OBTAINED_EXT:
            printk("DATE_TIME_OBTAINED_EXT\n");
            break;
        case DATE_TIME_NOT_OBTAINED:
            printk("DATE_TIME_NOT_OBTAINED\n");
            break;
        default:
            break;
    }

    /* Do not depend on obtained time, continue upon any event from the
     * date time library.
     */
    k_sem_give(&date_time_obtained);
}

int nrf_modem_get_time(void) {
    date_time_update_async(date_time_event_handler);

    int ret = k_sem_take(&date_time_obtained, K_SECONDS(DATE_TIME_TIMEOUT_S));
    if (ret) {
        printk("Date time, no callback event within %d seconds\n",
                DATE_TIME_TIMEOUT_S);
        return -ENODATA;
    }

    int64_t unix_time_ms;
    int err = date_time_now(&unix_time_ms);

    if (err) {
        printk("No time data from network yet...\n");
        return -ENODATA;
    }

    u32_t rtc_now_ms = k_uptime_get_32();
    rtc_offset_secs = (unix_time_ms - rtc_now_ms) / 1000;

    return 0;
}

const char* nrf_modem_get_imei(void) {
    if (!has_modem_params) {
        int err = modem_info_init();
        if (err) {
            printk("Unable to init modem params!\n");
            return NULL;
        }
        modem_info_params_init(&modem_params);
        err = modem_info_params_get(&modem_params);
        if (err) {
            printk("Unable to get modem IMEI\n");
            return NULL;
        }
        has_modem_params = true;
    }
    return modem_params.device.imei.value_string;
}