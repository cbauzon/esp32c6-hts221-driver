#include <stdio.h>
#include "driver/i2c_master.h"
#include "main.h"

void print_back(uint8_t addr, uint8_t data, rw read_or_write) {
    switch(read_or_write) {
        case READ:
            printf("(RD) Read data from 0x%02x: 0x%02x!\n", addr, data);
            break;
        
        case WRITE:
            printf("(WR) Successfully wrote 0x%02x to address 0x%02x!\n", addr, data);
            break;
        
        default:
            printf("(ERR) Error!");
    }
}



void read_reg(i2c_master_dev_handle_t dev_handle, uint8_t subaddr, uint8_t num_reads) {
    esp_err_t check;

    uint8_t data_rd[num_reads];
    check = i2c_master_transmit_receive(dev_handle, &subaddr, sizeof(uint8_t), &data_rd, num_reads*sizeof(uint8_t), -1);

    if (check == ESP_OK) {
        for (uint8_t i = 0; i<num_reads; ++i) {
            print_back(subaddr, data_rd[i], READ);
        }
    } else {
        printf("Failed to read from %02x!", subaddr);
    }
    
}

void who_am_i(i2c_master_dev_handle_t dev_handle) {
    uint8_t who_am_i_reg = 0x0f; 
    read_reg(dev_handle, who_am_i_reg, 1);
}

void write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t subaddr, uint8_t write_val) {
    esp_err_t check;

    uint8_t write_buffer[2] = {subaddr, write_val};

    check = i2c_master_transmit(i2c_dev, &write_buffer, 2*sizeof(uint8_t), -1);

    /* write back for confirmation */
    if (check == ESP_OK) {
        print_back(subaddr, write_val, WRITE);
    } else {
        printf("Failed to write to %02x!", subaddr);
    }
}

/*  function for multiple writes

    NO PRACTICAL USE! Wrote it to test my programming skills

*/
void write_reg_multiple(i2c_master_dev_handle_t i2c_dev, uint8_t subaddr, uint8_t write_vals[], uint32_t num_writes) {
    esp_err_t check;
    
    /* initialize write buffer */
    uint8_t write_buffer[num_writes+1];
    write_buffer[0] = subaddr;      // set the first data to subaddr
    for (uint32_t i=1; i<num_writes+1; ++i) {
        write_buffer[i] = write_vals[i-1];
    }

    /* transmit data onto bus*/
    check = i2c_master_transmit(i2c_dev, &write_buffer, (num_writes+1)*sizeof(uint8_t), -1);

    /* write back for confirmation */
    if (check == ESP_OK) {
        for (uint32_t i=0; i<num_writes; ++i) {
            print_back(subaddr, write_vals[i], WRITE);
        }
    } else {
        printf("Failed to write to %02x!", subaddr);
    }
        
}

void app_main(void)
{
    i2c_master_bus_handle_t i2c_mstr_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&I2C_MSTR_CFG, &i2c_mstr_handle));
    
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_mstr_handle, &DEV_CFG, &dev_handle));

    who_am_i(dev_handle);
    uint8_t write_vals[] = {0x83};
    write_reg_multiple(dev_handle, 0x20, write_vals, 1);
    read_reg(dev_handle, 0x20, 1);
    printf("Done!\n");
}