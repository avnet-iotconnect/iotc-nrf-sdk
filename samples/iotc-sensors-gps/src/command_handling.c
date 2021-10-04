
#include <zephyr.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if IS_ENABLED(CONFIG_BOARD_THINGY91_NRF9160NS)
#include "buzzer.h"
#include "led_pwm.h"
#else
#define ui_led_set_rgb(a,b,c)
#define ui_buzzer_set_frequency(a,b)
#endif
#include "iotconnect.h"
#include "app_common.h"

void command_status(IotclEventData data, bool status, const char *command_name, const char *message) {
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, status, message);
    printk("command: %s status=%s: %s\n", command_name, status ? "OK" : "Failed", message);
    printk("Sent CMD ack: %s\n", ack);
    iotconnect_sdk_send_packet(ack, NULL);
    free((void *) ack);
}


void process_command(IotclEventData data, char *args) {
    static const char* CMD_COLOR = "color";
    static const char* CMD_COLOR_RGB = "color-rgb";
    static const char* CMD_BUZZER_SET = "buzzer-set";
    static const char* CMD_BUZZER_BEEP = "buzzer-beep";
    static const char* CMD_BUZZER_TUNE = "buzzer-tune";

    static const char * SPACE = " ";
    char *command = strtok(args, SPACE);
    char *arg = strtok(NULL, SPACE);
    if (NULL == command) {
        printk("No command!\n");
        return;
    }
    if (0 == strcmp(CMD_COLOR_RGB, command)) {
        const char *cmd  = CMD_COLOR_RGB;
        if (NULL == arg) {
            command_status(data, false, cmd, "Missing argument");
            return;
        }
        int r, g, b;
        int num_found = sscanf(arg, "%02x%02x%02x", &r, &g, &b);
        if (3 != num_found) {
            command_status(data, false, cmd, "Argument format error!");
            return;
        }
        ui_led_set_rgb(r, g, b);
        command_status(data, true, cmd, "Success");
    } else if (0 == strcmp(CMD_COLOR, command)) {
        const char* cmd = CMD_COLOR;
        if (NULL == arg) {
            command_status(data, false, cmd, "Missing argument");
            return;
        }
        if (0 == strcmp("red", arg)) {
            ui_led_set_rgb(LED_MAX, 0, 0);
        } else if (0 == strcmp("green", arg)) {
            ui_led_set_rgb(0, LED_MAX, 0);
        } else if (0 == strcmp("blue", arg)) {
            ui_led_set_rgb(0, 0, 1);
        } else {
            command_status(data,
                           false,
                           cmd,
                           "Valid arguments are \"red\", \"green\" or \"blue\""
            );
            return;
        }
        command_status(data, true, cmd, "Success");
    } else if (0 == strcmp(CMD_BUZZER_SET, command)) {
        const char* cmd = CMD_BUZZER_SET;
        if (NULL == arg) {
            command_status(data, false, cmd, "Missing argument");
            return;
        }
        uint32_t frequency;
        int intensity;
        int num_found = sscanf(arg, "%u,%d", &frequency, &intensity);
        if (0 == num_found) {
            command_status(data, false, cmd, "Argument format: \"<frequency>,<intensity>\"!");
            return;
        } else if (1 == num_found) {
            intensity = 1;

        }  // else should be 2
        ui_buzzer_set_frequency(frequency, (uint8_t)intensity);
        command_status(data, true, cmd, "Success");
    } else if (0 == strcmp(CMD_BUZZER_BEEP, command)) {
        const char* cmd = CMD_BUZZER_BEEP;
        if (NULL == arg) {
            arg = "400,50";
        }
        uint32_t frequency;
        int intensity;
        int num_found = sscanf(arg, "%u,%d", &frequency, &intensity);
        if (0 == num_found) {
            command_status(data, false, cmd, "Argument format: \"<frequency>,<intensity>\"!");
            return;
        } else if (1 == num_found) {
            intensity = 1;

        }  // else should be 2
        ui_buzzer_set_frequency(frequency, (uint8_t)intensity);
        command_status(data, true, cmd, "Success");
        k_msleep(1000);
        ui_buzzer_set_frequency(0,0);
    } else if (0 == strcmp(CMD_BUZZER_TUNE, command)) {
        const char* cmd = CMD_BUZZER_TUNE;
        command_status(data, true, cmd, "Success");
        ui_buzzer_set_frequency(100, 20);
        k_msleep(150);
        ui_buzzer_set_frequency(400, 20);
        k_msleep(150);
        ui_buzzer_set_frequency(200, 20);
        k_msleep(150);
        ui_buzzer_set_frequency(500, 20);
        k_msleep(150);
        ui_buzzer_set_frequency(0, 0);
        k_msleep(150);
        ui_buzzer_set_frequency(500, 20);
        k_msleep(300);
        ui_buzzer_set_frequency(0, 0);

    } else {
        command_status(data, false, command, "Unknown command");
    }
}
