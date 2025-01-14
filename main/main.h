#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>

#define DEV_ADDR        0x5f
#define BUS_FREQ        100000

#define WHO_AM_I        0x0f
#define AV_CONF         0x10
#define CTRL_REG1       0x20
#define CTRL_REG2       0x21
#define CTRL_REG3       0x22
#define STATUS_REG      0x27
#define HUMIDITY_OUT_L  0x28
#define HUMIDITY_OUT_H  0x29
#define TEMP_OUT_L      0x2A
#define TEMP_OUT_H      0x2B

i2c_master_bus_config_t I2C_MSTR_CFG = {
    .i2c_port = -1,
    .sda_io_num = (GPIO_NUM_10),
    .scl_io_num = (GPIO_NUM_11),
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 0,
    .flags.enable_internal_pullup = 0
};

i2c_device_config_t DEV_CFG = {
    .dev_addr_length = I2C_ADDR_BIT_7,
    .device_address = DEV_ADDR,
    .scl_speed_hz = BUS_FREQ,
    .scl_wait_us = 0,
    .flags.disable_ack_check = 0

};

typedef enum rw {
    READ,
    WRITE
} rw;

typedef enum status {
    NOT_READY,
    TEMP_READY,
    HU_READY,
    BOTH_READY
} status;

void print_back(uint8_t addr, uint8_t data, rw read_or_write);
uint8_t * read_reg(i2c_master_dev_handle_t dev_handle, uint8_t subaddr, uint8_t num_reads, uint8_t alloc);
void who_am_i(i2c_master_dev_handle_t dev_handle);
void write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t subaddr, uint8_t write_val);
void write_reg_multiple(i2c_master_dev_handle_t i2c_dev, uint8_t subaddr, uint8_t write_vals[], uint32_t num_writes);
void master_init(void);

// typedef struct CTRL_REG1 {
//     uint8_t PD;         // 0: off, 1: on
//     uint8_t BDU;        // 0: continuous update, 1: update when read
//     uint8_t ODR;        // 0: one-shot, 1: 1Hz, 2: 7Hz, 3: 12.5Hz
// } CTRL_REG1;

#endif