#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "main.h"
#include "esp_heap_trace.h"
#include "esp_log.h"

void print_back(uint8_t addr, uint8_t data, rw read_or_write) {
    switch(read_or_write) {
        case READ:
            ESP_LOGI("RD","Read data from 0x%02x: 0x%02x!", addr, data);
            break;
        
        case WRITE:
            ESP_LOGI("WR", "Successfully wrote 0x%02x to address 0x%02x!", addr, data);
            break;
        
        default:
            ESP_LOGE("print_back", "Error!");
    }
}

uint8_t * read_reg(i2c_master_dev_handle_t dev_handle, uint8_t subaddr, uint8_t num_reads, bool alloc, bool print) {
    esp_err_t check;

    uint8_t *data_rd = (uint8_t *) malloc(num_reads*sizeof(uint8_t));
    check = i2c_master_transmit_receive(dev_handle, &subaddr, sizeof(uint8_t), data_rd, num_reads*sizeof(uint8_t), -1);

    if (check == ESP_OK) {

        if (print) {
            for (uint8_t i = 0; i<num_reads; ++i) {
                print_back(subaddr, data_rd[i], READ);
            }
        }
        
        // printf("read_reg return: %p\n", data_rd);

        if (alloc) return data_rd;
        else {
            free(data_rd);
            return NULL;
        }
    } else {
        ESP_LOGE("read_reg", "Failed to read from %02x!", subaddr);
    }
    free(data_rd);
    return NULL;
    
}

void who_am_i(i2c_master_dev_handle_t dev_handle) {
    read_reg(dev_handle, WHO_AM_I, 1, false, true);
}

void write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t subaddr, uint8_t write_val) {
    esp_err_t check;

    uint8_t write_buffer[2] = {subaddr, write_val};

    check = i2c_master_transmit(i2c_dev, &write_buffer, 2*sizeof(uint8_t), -1);

    /* write back for confirmation */
    if (check == ESP_OK) {
        print_back(subaddr, write_val, WRITE);
    } else {
        ESP_LOGE("write_reg", "Failed to write to %02x!", subaddr);
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
        ESP_LOGE("write_reg", "Failed to write to %02x!", subaddr);    
    }
        
}

sensor_status get_status(i2c_master_dev_handle_t dev_handle) {
    sensor_status res;
    uint8_t *data_rd = read_reg(dev_handle, STATUS_REG, 1, true, false);
    if (data_rd != NULL) {
        ESP_LOGD("get_status", "Status data addr: %p, %02x\n", data_rd, *data_rd);
    } else {
        ESP_LOGE("get_status", "Error reading from 0x27!");
    } 

    res = *data_rd;
    char *str_out = (char *)malloc(100*sizeof(char));

    switch (res) {
        case 0x00:
            strcpy(str_out, "NOT READY!");
            break;
        case 0x01:
            strcpy(str_out, "TEMP READY!");
            break;
        case 0x02:
            strcpy(str_out, "HUMIDITY READY!");
            break;
        case 0x03:
            strcpy(str_out, "BOTH READY!");
            break;

        default:
            ESP_LOGE("get_status", "Failed to get status!");
            break;
    }
    
    ESP_LOGI("get_status", "Status of the device is %s", str_out);
    free(data_rd);
    free(str_out);
    return res;
}

void get_hum_calibration(i2c_master_dev_handle_t dev_handle, calibration_hums *calib_hums, bool printout) {
    uint8_t *h0, *h0_out_l, *h0_out_h, 
            *h1, *h1_out_l, *h1_out_h;
    float h0_total, h1_total;
    int16_t h0_out, h1_out;

    // y variables
    h0 = read_reg(dev_handle, 0x30, 1, true, printout);
    h0_total = *h0 / 2.0;
    ESP_LOGD("get_hum_calibration", "h0_total is %0.2f%%", h0_total);
    calib_hums->h0_total = h0_total;

    h1 = read_reg(dev_handle, 0x31, 1, true, printout);
    h1_total = *h1 / 2.0;
    ESP_LOGD("get_hum_calibration", "h0_total is %0.2f%%", h1_total);
    calib_hums->h1_total = h1_total;

    // x variables
    h0_out_l = read_reg(dev_handle, 0x36, 1, true, printout);
    h0_out_h = read_reg(dev_handle, 0x37, 1, true, printout);
    h0_out = (*h0_out_h << 8) | *h0_out_l;
    ESP_LOGD("get_hum_calibration", "h0_out is 0x%04x", h0_out);
    calib_hums->h0_out = h0_out;

    h1_out_l = read_reg(dev_handle, 0x3a, 1, true, printout);
    h1_out_h = read_reg(dev_handle, 0x3b, 1, true, printout);
    h1_out = (*h1_out_h << 8) | *h1_out_l;
    ESP_LOGD("get_hum_calibration", "h1_out is 0x%04x", h1_out);
    calib_hums->h1_out = h1_out;

    free(h0);
    free(h0_out_l);
    free(h0_out_h);

    free(h1);
    free(h1_out_l);
    free(h1_out_h);
}

void get_hum(i2c_master_dev_handle_t dev_handle, calibration_hums *calib_hums, bool printout) {
    uint8_t *h_out_l, *h_out_h;
    float res_hum;
    int16_t h_out;

    // get temp reading
    h_out_l = read_reg(dev_handle, 0x28, 1, true, printout);
    h_out_h = read_reg(dev_handle, 0x29, 1, true, printout);
    h_out = ((*h_out_h << 8) | *h_out_l);
    ESP_LOGD("get_hum", "h_out is 0x%04x (unsigned) and 0x%04x (signed)", (uint16_t)h_out, h_out);

    res_hum =  (calib_hums->h1_total - calib_hums->h0_total) 
                / (calib_hums->h1_out - calib_hums->h0_out) 
                * (h_out - calib_hums->h0_out) + calib_hums->h0_total;
    ESP_LOGI("get_hum", "Humidity was calculated to be %0.2f %%rh", res_hum);

    free(h_out_l);
    free(h_out_h);
}

void get_temp_calibration(i2c_master_dev_handle_t dev_handle, calibration_temps *calib_temps, bool printout) {
    uint8_t *t0, *t0_out_l, *t0_out_h, 
            *t1, *t1_out_l, *t1_out_h, 
            *msb;
    float t0_total, t1_total;
    int16_t t0_out, t1_out;

    t0 = read_reg(dev_handle, 0x32, 1, true, printout);
    t1 = read_reg(dev_handle, 0x33, 1, true, printout);
    msb = read_reg(dev_handle, 0x35, 1, true, printout);

    // y variables
    t0_total = (float)(((*msb & 0x3) << 8) | *t0) / 8.0;
    ESP_LOGD("get_temp", "t0_total is %0.3f", t0_total);
    calib_temps->t0_total = t0_total;

    t1_total = (float)(((*msb & 0xC) << 6) | *t1) / 8.0;
    ESP_LOGD("get_temp", "t1_total is %0.3f", t1_total);
    calib_temps->t1_total = t1_total;

    // x variables
    t0_out_l = read_reg(dev_handle, 0x3c, 1, true, printout);
    t0_out_h = read_reg(dev_handle, 0x3d, 1, true, printout);
    t0_out = (*t0_out_h << 8) | *t0_out_l;
    ESP_LOGD("get_temp", "t0_out is 0x%04x", t0_out);
    calib_temps->t0_out = t0_out;

    t1_out_l = read_reg(dev_handle, 0x3e, 1, true, printout);
    t1_out_h = read_reg(dev_handle, 0x3f, 1, true, printout);
    t1_out = (*t1_out_h << 8) | *t1_out_l;
    ESP_LOGD("get_temp", "t1_out is 0x%04x", t1_out);
    calib_temps->t1_out = t1_out;

    free(t0);
    free(t0_out_l);
    free(t0_out_h);

    free(t1);
    free(t1_out_l);
    free(t1_out_h);

    free(msb);
}

void get_temp(i2c_master_dev_handle_t dev_handle, calibration_temps *calib_temps, bool printout) {
    uint8_t *t_out_l, *t_out_h;
    float res_temp;
    int16_t t_out;

    // get temp reading
    t_out_l = read_reg(dev_handle, 0x2a, 1, true, printout);
    t_out_h = read_reg(dev_handle, 0x2b, 1, true, printout);
    t_out = ((*t_out_h << 8) | *t_out_l);
    ESP_LOGD("get_temp", "t_out is 0x%04x (unsigned) and 0x%04x (signed)", (uint16_t)t_out, t_out);

    res_temp =  (calib_temps->t1_total - calib_temps->t0_total) 
                / (calib_temps->t1_out - calib_temps->t0_out) 
                * (t_out - calib_temps->t0_out) + calib_temps->t0_total;
    res_temp = (res_temp * 9/5) + 32;       // convert to fahrenheit
    ESP_LOGI("get_temp", "Temp was calculated to be %0.2f ºF", res_temp);

    free(t_out_l);
    free(t_out_h);

    
}

void app_main(void)
{
    #ifdef CONFIG_HEAP_TRACING_STANDALONE
        static heap_trace_record_t trace_record[100];

        ESP_ERROR_CHECK(heap_trace_init_standalone(trace_record, 100));
        ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_ALL));
    #endif
    calibration_temps *calib_temps = (calibration_temps *) malloc(sizeof(calibration_temps));
    calibration_hums *calib_hums = (calibration_hums *) malloc(sizeof(calibration_hums));

    i2c_master_bus_handle_t i2c_mstr_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&I2C_MSTR_CFG, &i2c_mstr_handle));
    
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_mstr_handle, &DEV_CFG, &dev_handle));

    who_am_i(dev_handle);
    // uint8_t write_vals[2] = {0x83, 0x81};
    // write_reg_multiple(dev_handle, CTRL_REG1, write_vals, 2);
    write_reg(dev_handle, CTRL_REG1, 0x83);
    write_reg(dev_handle, CTRL_REG2, 0x80);
    read_reg(dev_handle, CTRL_REG1, 1, false, true);
    read_reg(dev_handle, CTRL_REG2, 1, false, true);
    read_reg(dev_handle, CTRL_REG3, 1, false, true);
    read_reg(dev_handle, AV_CONF, 1, false, true);
    get_status(dev_handle);
    get_temp_calibration(dev_handle, calib_temps, false);
    get_hum_calibration(dev_handle, calib_hums, false);
    get_temp(dev_handle, calib_temps, false);
    get_hum(dev_handle, calib_hums, true);

    while (1) {
        sleep(2);
        get_temp(dev_handle, calib_temps, false);
        get_hum(dev_handle, calib_hums, false);

    }

    
    #ifdef CONFIG_HEAP_TRACING_STANDALONE
        ESP_ERROR_CHECK(heap_trace_stop());
        heap_trace_dump();
    #endif
    
    ESP_LOGI("app_main", "Done!");
}