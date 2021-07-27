
#define DT_DRV_COMPAT te_23142771

#include <drivers/sensor.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <logging/log.h>

#include "te23142771.h"


static te23142771_data_t te23142771_data;
static const te23142771_config_t te23142771_config = {
    .i2c_master_dev_name = DT_INST_BUS_LABEL(0),
    .i2c_slave_addr = DT_INST_REG_ADDR(0),
};

LOG_MODULE_REGISTER(TE23142771, CONFIG_SENSOR_LOG_LEVEL);



static int te23142771_sample_fetch(const struct device *dev,
                                    enum sensor_channel chan)
{
    te23142771_data_t *data = dev->data;
    const te23142771_config_t *config = dev->config;
    uint8_t reg_value;
    uint8_t rx[6];
    uint8_t rx_len;
    uint8_t rx_start_reg_addr;

    switch (chan) {
        case SENSOR_CHAN_AMBIENT_TEMP:
            reg_value = TE_2314277_1_REG_SCAN_TEMP_BITMASK;
            rx_len = 2;
            rx_start_reg_addr = TE_2314277_1_REG_TEMPERATURE_H;
            break;
        case SENSOR_CHAN_HUMIDITY:
            reg_value = TE_2314277_1_REG_SCAN_HUMIDITY_BITMASK;
            rx_len = 2;
            rx_start_reg_addr = TE_2314277_1_REG_HUMIDITY_H;
            break;
        case SENSOR_CHAN_LIGHT:
            reg_value = TE_2314277_1_REG_SCAN_LIGHT_BITMASK;
            rx_len = 2;
            rx_start_reg_addr = TE_2314277_1_REG_LIGHT_H;
            break;
        case SENSOR_CHAN_ALL:
            reg_value = TE_2314277_1_REG_SCAN_TEMP_BITMASK | \
                        TE_2314277_1_REG_SCAN_HUMIDITY_BITMASK | \
                        TE_2314277_1_REG_SCAN_LIGHT_BITMASK;
            rx_len = 6;
            rx_start_reg_addr = TE_2314277_1_REG_TEMPERATURE_H;
            break;
        default:
            return -ENOTSUP;
    }

    if (i2c_reg_write_byte(data->i2c_master, config->i2c_slave_addr, \
                            TE_2314277_1_REG_SCAN_START_BYTE, reg_value) < 0) {
        LOG_ERR("Failed to start sample.");
        return -EIO;
    }

    /* wait until measurement is complete */
    int err;
    do {
        k_sleep(K_MSEC(50));
        err = i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr, \
                                TE_2314277_1_REG_SCAN_START_BYTE, &reg_value);
    } while ((err == 0) && (reg_value > 0));
    if (err < 0) {
        LOG_ERR("Failed to start sample.");
        return err;        
    }

    if (i2c_burst_read(data->i2c_master, config->i2c_slave_addr, \
                        rx_start_reg_addr, rx, rx_len) < 0) {
        LOG_ERR("Failed to read sample");
        return -EIO;
    }

    data->sample_temperature = (rx[0] * 256.0 + rx[1]) / 10.0;
    data->sample_humidity = (rx[2] * 256.0 + rx[3]) / 10.0;
    data->sample_light = (rx[4] * 256.0 + rx[5]);

    return 0;
}

static inline void float_convert(struct sensor_value *val,
                                    float raw_val)
{
    /* Integer part of the value. */
    val->val1 = (int32_t)raw_val;

    /* Fractional part of the value (in one-millionth parts). */
    val->val2 = (int32_t)((raw_val - val->val1) * 1000000);
}

static int te23142771_channel_get(const struct device *dev,
                                    enum sensor_channel chan,
                                    struct sensor_value *val)
{
    te23142771_data_t *data = dev->data;

    switch (chan) {
        case SENSOR_CHAN_AMBIENT_TEMP:
            float_convert(val, data->sample_temperature);
            break;
        case SENSOR_CHAN_HUMIDITY:
            float_convert(val, data->sample_humidity);
            break;
        case SENSOR_CHAN_LIGHT:
            val->val1 = data->sample_light;
            val->val2 = 0;
            break;
        default:
            return -ENOTSUP;
    }

    return 0;
}

static int te23142771_init(const struct device *dev)
{
    const te23142771_config_t * const config = dev->config;
    te23142771_data_t *data = dev->data;

    data->i2c_master = device_get_binding(config->i2c_master_dev_name);
    if (data->i2c_master == NULL) {
        LOG_ERR("I2c master not found: %s", config->i2c_master_dev_name);
        return -EINVAL;
    }

    /* fail to read the option sensors reg if without adding delay from power on */
    k_sleep(K_SECONDS(3)); /* can't find start-up time in supplier materials; 
                                sleep value used derived from experimentation */

    /* read optional sensors register just to test the communication interface */
    uint8_t reg_value;
    if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr, \
                            TE_2314277_1_REG_OPTIONAL_SENSORS, &reg_value) < 0) {
        LOG_ERR("Failed to read optional sensors reg.");
        return -EIO;
    }

    return 0;
};


static const struct sensor_driver_api te23142771_driver_api = {
    .sample_fetch = te23142771_sample_fetch,
    .channel_get  = te23142771_channel_get,
};

DEVICE_AND_API_INIT(te23142771, DT_INST_LABEL(0), te23142771_init,
                    &te23142771_data, &te23142771_config, POST_KERNEL,
                    CONFIG_SENSOR_INIT_PRIORITY, &te23142771_driver_api);
