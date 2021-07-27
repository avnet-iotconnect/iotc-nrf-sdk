
#ifndef ZEPHYR_DRIVERS_SENSOR_TE23142771_TE23142771_H_
#define ZEPHYR_DRIVERS_SENSOR_TE23142771_TE23142771_H_


#include <zephyr/types.h>
#include <sys/util.h>
#include <device.h>


#define TE_2314277_1_REG_STATUS                 0x00
#define TE_2314277_1_REG_TEMPERATURE_H          0x01
#define TE_2314277_1_REG_TEMPERATURE_L          0x02
#define TE_2314277_1_REG_HUMIDITY_H             0x03
#define TE_2314277_1_REG_HUMIDITY_L             0x04
#define TE_2314277_1_REG_LIGHT_H                0x05
#define TE_2314277_1_REG_LIGHT_LOW              0x06
#define TE_2314277_1_REG_AUDIO_H                0x07
#define TE_2314277_1_REG_AUDIO_L                0x08
#define TE_2314277_1_REG_BATTERY_VOLTS_H        0x09
#define TE_2314277_1_REG_BATTERY_VOLTS_L        0x0A

#define TE_2314277_1_REG_FIRMWARE_VERSION       0x80
#define TE_2314277_1_REG_FIRMWARE_SUBVERSION    0x81
#define TE_2314277_1_REG_OPTIONAL_SENSORS       0x82

#define TE_2314277_1_REG_SCAN_START_BYTE        0xC0
#define TE_2314277_1_REG_RESET                  0xF0

#define TE_2314277_1_RESET                      0xA5

#define TE_2314277_1_OPTIONAL_SENSORS_NONE          0x00
#define TE_2314277_1_OPTIONAL_CO2_BITMASK           BIT(0)
#define TE_2314277_1_OPTIONAL_MIC_BITMASK           BIT(2)

#define TE_2314277_1_REG_SCAN_STATUS_BITMASK        BIT(0)
#define TE_2314277_1_REG_SCAN_TEMP_BITMASK          BIT(1)
#define TE_2314277_1_REG_SCAN_HUMIDITY_BITMASK      BIT(2)
#define TE_2314277_1_REG_SCAN_LIGHT_BITMASK         BIT(3)
#define TE_2314277_1_REG_SCAN_AUDIO_BITMASK         BIT(4)
#define TE_2314277_1_REG_SCAN_BATT_BITMASK          BIT(5)
#define TE_2314277_1_REG_SCAN_GAS_BITMASK           BIT(6)

#define TE_2314277_1_REG_STATUS_MOTION_EVT_BITMASK  BIT(0)
#define TE_2314277_1_REG_STATUS_AUDIO_EVT_BITMASK   BIT(1)
#define TE_2314277_1_REG_STATUS_PIR_EVT_BITMASK     BIT(7)


typedef struct {
    char *i2c_master_dev_name;
    uint16_t i2c_slave_addr;
} te23142771_config_t;

typedef struct {
    const struct device *i2c_master;

    float sample_temperature;
    float sample_humidity;
    uint16_t sample_light;
} te23142771_data_t;


#endif /* ZEPHYR_DRIVERS_SENSOR_TE23142771_TE23142771_H_ */
