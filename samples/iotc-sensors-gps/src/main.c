#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
//
// Copyright: Avnet, Softweb Inc. 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/15/20.
// Modified by Shu Liu <shu.liu@avnet.com>
//

#include <stdlib.h>
#include <stdio.h>

#include <zephyr.h>

#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>

#include <power/reboot.h>
#include <dfu/mcuboot.h>
#include <drivers/gps.h>
#if IS_ENABLED(CONFIG_BOARD_THINGY91_NRF9160NS)
#include "led_pwm.h"
#include "buzzer.h"
#else
#define ui_leds_init()
#define ui_led_set_rgb(a,b,c) (void) (a,b,c)
#define ui_buzzer_init()
#endif
#include "iotconnect.h"
#include "nrf_modem_if.h"
#if IS_ENABLED(CONFIG_BH1749)
#include "light_sensor.h"
#else
#define light_sensor_init()
#endif
#include "env_sensors.h"
#include "motion.h"
#include "gps_controller.h"
#include "iotconnect_common.h"
#include "iotconnect_telemetry.h"
#include "iotconnect_lib.h"
#include "dk_buttons_and_leds.h"

#include "nrf_cert_store.h"
#include "nrf_fota.h"
#include "app_common.h"

#if defined(CONFIG_PROVISION_TEST_CERTIFICATES)
#include "test_certs.h"
#endif

#define PRINT_LTE_LC_EVENTS

#define SDK_VERSION STRINGIFY(APP_VERSION)
#define MAIN_APP_VERSION "01.02.00" // Use two-digit or letter version so that we can use strcmp to see if version is greater

static enum lte_lc_system_mode default_system_mode;
static enum lte_lc_system_mode_preference preference_mode;

static char duid[65] = CONFIG_IOTCONNECT_CUSTOM_DUID; // When using this code, your device ID will be nrf-IMEI.
static char *cpid = CONFIG_IOTCONNECT_CPID;
static char *env = CONFIG_IOTCONNECT_ENV;

static IotconnectNrfFotaConfig fota_config = {0};

// Various flags that drive the behavior of main loop
static bool sdk_running = false;
static bool sdk_do_run = true;    // trigger running of the SDK - by default and by button press
static bool sdk_do_shutdown = false;  // trigger stopping of the SDK loop
static bool do_reboot = false;
static bool gps_running = false;
static bool gps_do_run = false;
static bool gps_do_stop = false;
static bool fota_in_progress = false;
static bool lte_link_up = false;
static bool connecting_to_iotconnect = false;
static bool time_updated = false;

#define INVALID_LAT_LON -1000.0f
static float gps_lat = INVALID_LAT_LON;
static float gps_lng = INVALID_LAT_LON;

static int setup_modem_gps(void);


#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)

/* Initialize AT communications */
static int at_comms_init(void) {
    int err;
    err = at_cmd_init();
    if (err) {
        printk("Failed to initialize AT commands, err %d\n", err);
        return err;
    }
    err = at_notif_init();
    if (err) {
        printk("Failed to initialize AT notifications, err %d\n", err);
        return err;
    }
    return 0;
}

#endif

static bool is_app_version_same_as_ota(const char *version) {
    return strcmp(MAIN_APP_VERSION, version) == 0;
}

static bool app_needs_ota_update(const char *version) {
    return strcmp(MAIN_APP_VERSION, version) < 0;
}

static void nrf_fota_cb(const struct fota_download_evt *evt) {
    switch (evt->id) {
        case FOTA_DOWNLOAD_EVT_FINISHED:
            printk("OTA: Download finished. Board reboot is scheduled...\n");
            fota_in_progress = false;
            if (sdk_running) {
                sdk_do_shutdown = true;
            }
            do_reboot = true;
            break;
        case FOTA_DOWNLOAD_EVT_ERROR:
            // Even if we get an error, we can't do anything about it other than try again
            printk("OTA: Download error!\n");
            fota_in_progress = false;
            if (!sdk_running) {
                sdk_do_run = true;
            }
        default:
            // everything else is already handled by the fota module (prints messages)
            break;
    }
}

// Parses the URL into host and path strings.
// It re-uses the URL storage by splitting it into two null-terminated strings.
static int start_ota(char *url) {
    size_t url_len = strlen(url);
    int slash_count = 0;
    for (int i = 0; i < url_len; i++) {
        if (url[i] == '/') {
            slash_count++;
            if (slash_count == 2) {
                fota_config.host = &url[i + 1];
            } else if (slash_count == 3) {
                url[i] = 0; // terminate the host string
                fota_config.path = &url[i + 1];
                fota_config.fota_cb = nrf_fota_cb;
                fota_config.apn = NULL;
                return nrf_fota_start(&fota_config);
            }
        }
    }
    return -EINVAL;
}

static void on_ota(IotclEventData data) {
    const char *message = NULL;
    char *url = iotcl_clone_download_url(data, 0);
    bool success = false;
    if (NULL != url) {
        printk("Download URL is: %s\n", url);
        const char *version = iotcl_clone_sw_version(data);
        if (is_app_version_same_as_ota(version)) {
            printk("OTA request for same version %s. Sending success\n", version);
            success = true;
            message = "Version is matching";
        } else if (app_needs_ota_update(version)) {
            int err = start_ota(url);
            if (err) {
                printk("Failed to start OTA. Error was %d\n", err);
                message = "Failed to start OTA";
            } else {
                // Don't send ack yet.
                // Wait for OTA to process, reboot and wait for a new OTA request (because we didn't respond)
                // Don't free URL and version. We will use them for OTA download
                fota_in_progress = true;
                ui_led_set_rgb(LED_MAX, LED_MAX, 0);  //yellow..
                return;
            }
        } else {
            printk("Device firmware version %s is newer than OTA version %s. Sending failure\n", MAIN_APP_VERSION,
                   version);
            // Not sure what to do here. The app version is better than OTA version.
            // Probably a development version, so return failure?
            // The user should decide here.
            success = false;
            message = "Device firmware version is newer";
        }

        free((void *) url);
        free((void *) version);
    } else {
        // compatibility with older events
        // This app does not support FOTA with older back ends, but the user can add the functionality
        const char *command = iotcl_clone_command(data);
        if (NULL != command) {
            // URL will be inside the command
            printk("Command is: %s\n", command);
            message = "Back end version 1.0 OTA not supported by the app";
            free((void *) command);
        }
    }
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, success, message);
    if (NULL != ack) {
        printk("Sent OTA ack: %s\n", ack);
        iotconnect_sdk_send_packet(ack);
        free((void *) ack);
    }
}

static void on_command(IotclEventData data) {
    char *command = iotcl_clone_command(data);
    if (NULL != command) {
        printk("Received command: %s\n", command);
        process_command(data, command);
        free((void *) command);
    } else {
        command_status(data, false, "?", "Internal error");
    }
}

static void on_connection_status(IotconnectConnectionStatus status) {
    // Add your own status handling
    switch (status) {
        case MQTT_CONNECTED:
            printk("IoTConnect MQTT Connected\n");
            ui_led_set_rgb(0, LED_MAX, 0);

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
            /* Mark image as good to avoid rolling back after update
             * The last image was online when downloaded, so this image better get online too
             * */
            boot_write_img_confirmed();
#endif
            break;
        case MQTT_DISCONNECTED:
            printk("IoTConnect MQTT Disconnected\n");
            ui_led_set_rgb(LED_MAX, 0, 0);
            break;
        case MQTT_FAILED:
        default:
            printk("IoTConnect MQTT ERROR\n");
            ui_led_set_rgb(0, LED_MAX, 0);
            break;
    }
}

static void publish_telemetry() {
    IotclMessageHandle msg = iotcl_telemetry_create(iotconnect_sdk_get_lib_config());

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_string(msg, "version", MAIN_APP_VERSION);
    iotcl_telemetry_set_string(msg, "api_version", SDK_VERSION);

    iotcl_telemetry_set_number(msg, "cpu", 33);

#if IS_ENABLED(CONFIG_BH1749)
    {
        light_sensor_data_t ld = {0};
        if (0 == light_sensor_get_data(&ld)) {
            iotcl_telemetry_set_number(msg, "ls_red", ld.red);
            iotcl_telemetry_set_number(msg, "ls_green", ld.green);
            iotcl_telemetry_set_number(msg, "ls_blue", ld.blue);
            iotcl_telemetry_set_number(msg, "ls_ir", ld.ir);
        }
    }
#endif
    {
        env_sensor_data_t ed = {0};
        if (0 == env_sensors_get_data(&ed)) {
            iotcl_telemetry_set_number(msg, "es_temp", ed.temperature);
            iotcl_telemetry_set_number(msg, "es_humid", ed.humidity);
            iotcl_telemetry_set_number(msg, "es_pres", ed.pressure);
        } else {
          printk("UNABLE TO READ ENVIRONMENT DATA\n");
        }
    }
    {
        motion_data_t md = {0};
        if (0 == accelerometer_get_data(&md)) {
            iotcl_telemetry_set_number(msg, "ms_x", md.acceleration.x);
            iotcl_telemetry_set_number(msg, "ms_y", md.acceleration.y);
            iotcl_telemetry_set_number(msg, "ms_z", md.acceleration.z);
            char *orientation = "unknown";
            switch (md.orientation) {
                case MOTION_ORIENTATION_NORMAL:
                    orientation = "up";
                    break;
                case MOTION_ORIENTATION_UPSIDE_DOWN:
                    orientation = "down";
                    break;
                case MOTION_ORIENTATION_ON_SIDE:
                    orientation = "side";
                    break;
                default:
                    break;
            }
            iotcl_telemetry_set_string(msg, "ms_orientation", orientation);
        }

        if (gps_lat != INVALID_LAT_LON && gps_lng != INVALID_LAT_LON) {
            iotcl_telemetry_set_number(msg, "gps_lat", gps_lat);
            iotcl_telemetry_set_number(msg, "gps_lng", gps_lng);
        }
    }
    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    printk("Sending: %s\n", str);
    iotconnect_sdk_send_packet(str);
    iotcl_destroy_serialized(str);
}

static int time_init() {
    int err;
    for (int tries = 0; tries < 5; tries++) {
        err = nrf_modem_get_time();
        if (err) {
            printk("Retrying to get time...\n");
            k_msleep(3000);
        } else {
            return 0;
        }
    }
    printk("Failed to initialize time!\n");
    return -ETIMEDOUT;
}

static void print_lte_lc_evt_string(const struct lte_lc_evt *const evt) {

#ifdef PRINT_LTE_LC_EVENTS

    switch (evt->type) {

        case LTE_LC_EVT_NW_REG_STATUS:

            switch ((int)evt->nw_reg_status) {

                case LTE_LC_NW_REG_REGISTERED_HOME:
                    printk("LTE network registered (Home)\n");
                    break;

                case LTE_LC_NW_REG_REGISTERED_ROAMING:
                    printk("LTE network registered (Roaming)\n");
                    break;

                case LTE_LC_NW_REG_SEARCHING:
                    printk("LTE network searching...\n");
                    break;

                case LTE_LC_NW_REG_NOT_REGISTERED:
                    printk("LTE network not registered\n");
                    break;

                case LTE_LC_NW_REG_REGISTRATION_DENIED:
                    printk("LTE network registered (HOME)\n");
                    break;

                case LTE_LC_NW_REG_UNKNOWN:
                    printk("LTE network registration status unknown\n");
                    break;

                case LTE_LC_NW_REG_REGISTERED_EMERGENCY:
                    printk("LTE network registered (Emergency)\n");
                    break;

                case LTE_LC_NW_REG_UICC_FAIL:
                    printk("SIM card UICC read fail\n");
                    break;
            }
            break;

        case LTE_LC_EVT_PSM_UPDATE:
            printk("PSM update: active time => %d, TAU => %d\n", evt->psm_cfg.active_time, evt->psm_cfg.tau);
            break;

        case LTE_LC_EVT_EDRX_UPDATE:
            printk("eDRX update: eDRX => %fs, PTW => %fs\n", evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
            break;

        case LTE_LC_EVT_RRC_UPDATE:
            printk("RRC update: mode => %s\n", (evt->rrc_mode == LTE_LC_RRC_MODE_IDLE ? "idle":"connected"));
            break;

        case LTE_LC_EVT_CELL_UPDATE:
            printk("Cell id => 0x%08X, Cell tac => 0x%08X\n", evt->cell.id, evt->cell.tac);
            break;

        case LTE_LC_EVT_LTE_MODE_UPDATE:
            printk("Received LTE_LC_EVT_LTE_MODE_UPDATE\n");
            break;

        case LTE_LC_EVT_TAU_PRE_WARNING:
            printk("Received LTE_LC_EVT_TAU_PRE_WARNING\n");
            break;

        case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
            printk("Received LTE_LC_EVT_NEIGHBOR_CELL_MEAS\n");
            break;

        case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
            printk("Received LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING\n");
            break;

        case LTE_LC_EVT_MODEM_SLEEP_EXIT:
            printk("Received LTE_LC_EVT_MODEM_SLEEP_EXIT\n");
            break;

        case LTE_LC_EVT_MODEM_SLEEP_ENTER:
            printk("Received LTE_LC_EVT_MODEM_SLEEP_ENTER\n");
            break;
    }

#endif    
}

static void nrf_lte_evt_cb(const struct lte_lc_evt *const evt) {

    static enum lte_lc_nw_reg_status s_lte_nw_reg_status = LTE_LC_NW_REG_UNKNOWN;

    print_lte_lc_evt_string(evt);

    switch(evt->type){

        case LTE_LC_EVT_NW_REG_STATUS:

            s_lte_nw_reg_status = evt->nw_reg_status;

            switch((int)evt->nw_reg_status){

                case LTE_LC_NW_REG_REGISTERED_HOME:
                case LTE_LC_NW_REG_REGISTERED_ROAMING:
                    lte_link_up = true;
                    break;

                case LTE_LC_NW_REG_SEARCHING:
                case LTE_LC_NW_REG_NOT_REGISTERED:
                case LTE_LC_NW_REG_REGISTRATION_DENIED:
                case LTE_LC_NW_REG_UNKNOWN:
                case LTE_LC_NW_REG_REGISTERED_EMERGENCY:
                case LTE_LC_NW_REG_UICC_FAIL:
                    lte_link_up = false;
                    break;
            }
            break;

        case LTE_LC_EVT_CELL_UPDATE:

            if (evt->cell.id == 0xFFFFFFFF) {

                if (lte_link_up) {

                    lte_link_up = false;
                    printk("LTE network down\n");
                }

            } else if (!lte_link_up) {

                if(s_lte_nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
                    s_lte_nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {

                    lte_link_up = true;
                    printk("LTE network established\n");
                }
            }
            break;

        case LTE_LC_EVT_PSM_UPDATE:
        case LTE_LC_EVT_EDRX_UPDATE:
        case LTE_LC_EVT_RRC_UPDATE:
        case LTE_LC_EVT_LTE_MODE_UPDATE:
        case LTE_LC_EVT_TAU_PRE_WARNING:
        case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
        case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
        case LTE_LC_EVT_MODEM_SLEEP_EXIT:
        case LTE_LC_EVT_MODEM_SLEEP_ENTER:

            break;
    }

    if (lte_link_up) {
        ui_led_set_rgb(LED_MAX, 0, LED_MAX);
    } else {
        ui_led_set_rgb(LED_MAX, LED_MAX, 0);
    }
}

static void lte_link_and_connection_handler() {

    int err;

    while (!lte_link_up || !time_updated ||
            (!iotconnect_sdk_is_connected() && !connecting_to_iotconnect)) {

        if (lte_link_up) {

            if (!time_updated) {

                printk("perform time update\n");

                //perform time update
                err = time_init();
                if (err) {
                    printk("time_init() return err %d\n", err);
                    k_msleep(100);
                    continue;
                }

                time_updated = true;
            }

            //connect to IotConnect if not.
            if (!iotconnect_sdk_is_connected() && !connecting_to_iotconnect) {

                connecting_to_iotconnect = true;
                err = iotconnect_sdk_init();
                if (err) {
                    printk("Failed to connect to IoTConnect MQTT broker, err %d", err);
                    sdk_running = false;
                    connecting_to_iotconnect = false;
                    k_msleep(1000);
                } else {
                    ui_led_set_rgb(0, LED_MAX, LED_MAX);
                }
            }
        } else {
            if (iotconnect_sdk_is_connected()) {
                iotconnect_sdk_abort();
            }
        }
        k_msleep(1);
    }

    if (iotconnect_sdk_is_connected() && connecting_to_iotconnect) {

        connecting_to_iotconnect = false;
    }
}

//#define MEMORY_TEST
#ifdef MEMORY_TEST
#define TEST_BLOCK_SIZE  1024
#define TEST_BLOCK_COUNT 100
static void memory_test() {
    static void *blocks[TEST_BLOCK_COUNT];
    int i = 0;
    for (; i < TEST_BLOCK_COUNT; i++) {
        void *ptr = malloc(TEST_BLOCK_SIZE);
        if (!ptr) {
            break;
        }
        blocks[i] = ptr;
    }
    printk("====Allocated %d blocks of size %d (of max %d)===\n", i, TEST_BLOCK_SIZE, TEST_BLOCK_COUNT);
    for (int j = 0; j < i; j++) {
        free(blocks[j]);
    }
}
#endif /* MEMORY_TEST */

static int sdk_run() {
    int err;
    sdk_running = true;
    time_t now = 0;
    time_t last_send_time = 0;
    time_t stop_send_time = 0;

#ifdef MEMORY_TEST
    // uncomment if you want to check memory
    // we should have approximately the same amount of RAM every time we come in here
    // The number of allocated blocks may vary a bit due to fragmentation
    memory_test();
#endif /* MEMORY_TEST */

    connecting_to_iotconnect = false;
    time_updated = false;

    err = lte_lc_connect_async(nrf_lte_evt_cb);
    if (err) {
        printk("Failed to connect to the LTE network, err %d\n", err);
        sdk_running = false;
        return err;
    } else {
        printk("Start establishing LTE network...\n");
    }

    do {

        lte_link_and_connection_handler();

        // measure time
        if (!now) {
            now = time(NULL);
            last_send_time = now - CONFIG_TELEMETRY_SEND_INTERVAL_SECS; // send data every 10 seconds
            stop_send_time = now + 60 * CONFIG_TELEMETRY_DURATION_MINUTES; // stop sending after a few minutes
        }

        iotconnect_sdk_loop();
        if (sdk_do_shutdown) {
            sdk_do_shutdown = false;
            break;
        }

        now = time(NULL);
        if (iotconnect_sdk_is_connected() && now - last_send_time >= CONFIG_TELEMETRY_SEND_INTERVAL_SECS) {
            last_send_time = now;
            if (!fota_in_progress) {
                publish_telemetry();

            }
        }
        if (fota_in_progress) {
            // extend telemetry duration to a full interval, just to keep things connected and avoid disconnection
            stop_send_time = now + 60 * CONFIG_TELEMETRY_DURATION_MINUTES;
        }
        k_msleep(CONFIG_MAIN_LOOP_INTERVAL_MS);
        now = time(NULL);
    } while (CONFIG_TELEMETRY_DURATION_MINUTES >= 0 && now < stop_send_time);

    // this function will stop the IoTConnect SDK
    if (iotconnect_sdk_is_connected()) {
        iotconnect_sdk_disconnect();
        k_msleep(CONFIG_MAIN_LOOP_INTERVAL_MS);
        iotconnect_sdk_loop();
        k_msleep(CONFIG_MAIN_LOOP_INTERVAL_MS);
    } else {
        iotconnect_sdk_abort();
    }
    if (!fota_in_progress) {
        // special case. don't go offline here. let fota do its thing
        lte_lc_offline();
        ui_led_set_rgb(0, 0, 0);
    } else {
        printk("-----AWAITING OTA----\n");
    }
    sdk_running = false;
    return 0;
}

static void button_handler(uint32_t button_state, uint32_t has_changed) {
    static int64_t time_button_down = 0;

    if (has_changed && (button_state & 1U) == 1U) {
        time_button_down = k_uptime_get();
    } else if (has_changed && time_button_down > 0 && (button_state & 1U) == 0) {
        int32_t time_diff = (int32_t) (k_uptime_get() - time_button_down);
        if (time_diff > 10 && time_diff < 300) {
            // short press
            if (sdk_running) {
                sdk_do_shutdown = true;
            } else if (gps_running) {
                sdk_do_run = true;
                gps_do_stop = true;
            } else {
                sdk_do_run = true;
            }
        } else if (time_diff > 500 && time_diff < 8000) {
            if (fota_in_progress) {
                printk("Not reacting to long press because fota is in progress\n");
                return;
            }
            // long press
            if (sdk_running) {
                sdk_do_shutdown = true;
                gps_do_run = true;
            } else if (gps_running) {
                // same as short press if running
                gps_do_stop = true;
                sdk_do_run = true;
            } else {
                // case where sdk is shut down
                gps_do_run = true;
            }
        }
    }
}

static void gps_evt_handler(const struct device *dev, struct gps_event *evt) {
    (void) dev;
    switch (evt->type) {
        case GPS_EVT_SEARCH_STARTED:
            printk("GPS_EVT_SEARCH_STARTED\n");
            break;
        case GPS_EVT_SEARCH_STOPPED:
            printk("GPS_EVT_SEARCH_STOPPED\n");
            break;
        case GPS_EVT_SEARCH_TIMEOUT:
            printk("GPS_EVT_SEARCH_TIMEOUT\n");
            break;
        case GPS_EVT_PVT:
            /* Don't spam logs */
            break;
        case GPS_EVT_PVT_FIX:
            printk("GPS_EVT_PVT_FIX\n");
            break;
        case GPS_EVT_NMEA:
            /* Don't spam logs */
            break;
        case GPS_EVT_NMEA_FIX: {
            struct gps_nmea gps_data;
            printk("Position fix with NMEA data\n");
            ui_led_set_rgb(0, 0, LED_MAX);

            memcpy(gps_data.buf, evt->nmea.buf,
                   evt->nmea.len);     //nmea gps data, latitude: ddmm.mmmmm; longtitude: dddmm.mmmmm
            gps_data.len = evt->nmea.len;


            char gps_latitude0[2];
            char gps_latitude1[8];
            char gps_latitude_direction;
            memcpy(gps_latitude0, &gps_data.buf[17], 2);
            memcpy(gps_latitude1, &gps_data.buf[19], 8);
            gps_latitude_direction = gps_data.buf[28];
            float lat0 = (float) atof(gps_latitude0);
            float lat1 = (float) atof(gps_latitude1);

            if (gps_latitude_direction == 'N') {                 //78 is N
                gps_lat = lat0 + (lat1 / 60);
            } else if (gps_latitude_direction == 'S') {            //83 is S
                gps_lat = (lat0 + (lat1 / 60)) * (-1);
            } else {
                gps_lat = INVALID_LAT_LON;
            }

            char gps_longtitude0[3];
            char gps_longtitude1[8];
            char gps_longtitude_direction;
            memcpy(gps_longtitude0, &gps_data.buf[30], 3);
            memcpy(gps_longtitude1, &gps_data.buf[33], 8);
            gps_longtitude_direction = gps_data.buf[42];
            float lng0 = (float) atof(gps_longtitude0);
            float lng1 = (float) atof(gps_longtitude1);

            if (gps_longtitude_direction == 'E') {               //69 is E
                gps_lng = lng0 + (lng1 / 60);
            } else if (gps_longtitude_direction == 'W') {        //87 is W
                gps_lng = (lng0 + (lng1 / 60)) * (-1);
            } else {
                gps_lng = INVALID_LAT_LON;
            }
//                printk("lat2 is %d\n", gps_latitude2);
//                printk("lng2 is %d\n", gps_longtitude2);
//                printf("..............GPS Latitude is %f\n", gps_lat);             
//                printf("..............GPS Longtitude is %f\n", gps_lng);

            gps_do_stop = true;
            sdk_do_run = true;
            break;
        }
        case GPS_EVT_OPERATION_BLOCKED:
            printk("GPS_EVT_OPERATION_BLOCKED\n");
            break;
        case GPS_EVT_OPERATION_UNBLOCKED:
            printk("GPS_EVT_OPERATION_UNBLOCKED\n");
            break;
        case GPS_EVT_AGPS_DATA_NEEDED:
            printk("GPS_EVT_AGPS_DATA_NEEDED\n");
            /* Send A-GPS request with short delay to avoid LTE network-
             * dependent corner-case where the request would not be sent.
             */
            // AGPS code here.
            /*
            static struct gps_agps_request agps_request;
            memcpy(&agps_request, &evt->agps_request, sizeof(agps_request));
            lte_lc_system_mode_set(default_system_mode); // mixed with gps
            int err = gps_agps_request(agps_request, GPS_SOCKET_NOT_PROVIDED);
            if (err) {
                printk("AGPS could not be requested\n");
            } else {
                printk("AGPS request success\n");
            }
            lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_GPS);
            */

            break;
        case GPS_EVT_ERROR:
            printk("GPS_EVT_ERROR\n");
            break;
        default:
            break;
    }
}


static void gps_handler() {
    printk("Starting GPS\n");

    if (setup_modem_gps() != 0) {
        printk("Failed to initialize modem for GPS\n");
    }
    ui_led_set_rgb(LED_MAX, LED_MAX, LED_MAX);
    gps_control_start();
}


static int setup_modem_gps(void) {
    printk("turn on modem to GPS mode\n");
    lte_lc_offline();
    lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_GPS, preference_mode);
    lte_lc_normal();
    return 0;
}

void main(void) {
    int err;

    printk("Starting IoTConnect SDK Demo %s\n", MAIN_APP_VERSION);
    ui_leds_init();
    k_msleep(10); // let PWM initialize
    ui_led_set_rgb(LED_MAX, LED_MAX, 0);

    k_msleep(4000); // allow time for the user to connect the comm port to see the generated DUID and initialization errors

#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
    err = nrf_modem_lib_init();
#else
    /* If bsdlib is initialized on post-kernel we should
     * fetch the returned error code instead of nrf_modem_lib_init
     */
    err = nrf_modem_lib_get_init_ret();
#endif
    if (err) {
        printk("Failed to initialize nrf_modem_lib!\n");
        return;
    }

#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
    err = at_comms_init();
    if (err) {
        printk("Failed to initialize modem!\n");
        return;
    }
#endif
    gps_control_init(gps_evt_handler);

    err = nrf_cert_store_provision_api_certs();
    if (err) {
        printk("Failed to provision API certificates!\n");
        return;
    }

    err = nrf_cert_store_provision_ota_certs();
    if (err) {
        printk("Failed to provision OTA certificates!\n");
        return;
    }

    err = lte_lc_init();
    if (err) {
        printk("Failed to initialize the modem, err %d\n", err);
        return;
    }

    err = lte_lc_system_mode_get(&default_system_mode, &preference_mode);
    if (err) {
        printk("Failed to get system mode %d\n", err);
        return;
    }

    err = nrf_fota_init();
    if (err) {
        printk("Failed to initialize the OTA module, err %d\n", err);
        return;
    }

    const char *imei = nrf_modem_get_imei();

    if (!imei) {
        printk("Unable to obtain IMEI from the board!\n");
        return;
    }

#if defined(CONFIG_PROVISION_TEST_CERTIFICATES)
    /*
    if(nrf_cert_store_delete_all_device_certs()) {
        printk("Failed to delete device certs\n");
    } else {
        printk("Device certs deleted\n");
    }
     */
    if (program_test_certs(env, imei)) {
        printk("Failed program certs. Error was %d. Assuming certs are already programmed.\n", err);
    } else {
        printk("Device provisioned successfully\n");
    }
#endif
    if (strlen(CONFIG_IOTCONNECT_CUSTOM_DUID) == 0) {
        strcpy(duid, "nrf-");
        strcat(duid, imei);
    }
    printk("DUID: %s\n", duid);

    light_sensor_init();
    env_sensors_init();
    accelerometer_init();
    ui_buzzer_init();
    dk_buttons_init(button_handler);

    if (strlen(cpid) == 0 || strlen(env) == 0) {
        printk("You must configure your CPID and ENV in Kconfig\n");
        printk("If using Segger Embedded Studio, go to Project->Configure nRF Connect SDK Project\n");
        printk("And configure Company ID and Environment values.\n");
        printk("Contact your IoTConnect representative in you need help with configuring the device.\n");
        return;
    }

    IotconnectClientConfig *config = iotconnect_sdk_init_and_get_config();
    config->cpid = cpid;
    config->duid = duid;
    config->env = env;
    config->cmd_cb = on_command;
    config->ota_cb = on_ota;
    config->status_cb = on_connection_status;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        if (gps_running && gps_do_stop) {
            gps_do_stop = false;
            gps_control_stop();
            gps_running = false;
            lte_lc_offline();
            lte_lc_system_mode_set(default_system_mode, preference_mode);
        }
        if (sdk_do_run && !sdk_running) {
            sdk_do_run = false;

            if (!sdk_run()) {
                ui_led_set_rgb(LED_MAX, 0, 0);
                k_msleep(1000);
                ui_led_set_rgb(0, 0, 0);
            }
        }
        if (!sdk_running && gps_do_run && !gps_running) {
            gps_do_run = false;
            gps_running = true;
            lte_lc_offline();
            gps_handler();
        }
        if (do_reboot) {
            printk("The board will reboot in 2 seconds\r\n");
            do_reboot = false; // pointless, but just in case...
            k_msleep(2000);
            sys_reboot(SYS_REBOOT_COLD);
        }
        k_msleep(100);
    }
#pragma clang diagnostic pop

}

