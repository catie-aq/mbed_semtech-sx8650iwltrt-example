/* mbed Microcontroller Library
 * Copyright (c) 2020 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "BlockDevice.h"
#include "FlashIAPBlockDevice.h"
#include "ili9163c.h"
#include "mbed.h"
#include "sx8650iwltrt.h"
#include <errno.h>

using namespace sixtron;

#define FLASH_ENABLE 1 // state of led flash during the capture
#define FLASHIAP_ADDRESS_H753ZI 0x08100000
#define FLASHIAP_ADDRESS_L4A6RG 0x08080000
#define FLASHIAP_SIZE 0x20000

/* Driver Touchscreen */
#define SX8650_SWITCHED_TIME 5ms

// Protoypes
void draw_cross(uint8_t x, uint8_t y);
void draw(uint16_t x, uint16_t y);
void read_coordinates(uint16_t x, uint16_t y);
void read_pressures(uint16_t z1, uint16_t z2);
void clear_settings(SX8650IWLTRT::coefficient *settings);
int read_configuration(SX8650IWLTRT::coefficient *coef);
int write_configuration(SX8650IWLTRT::coefficient *coef);
void application_setup(void);
void retrieve_calibrate_data(char *bufer);
void save_calibrate_date(char *buffer);
void lcd_calibration();
void button_handler();

// RTOS
Thread thread;
Thread thread_button;
static EventQueue event_queue;
static EventQueue event_queue_button;

// Preipherals
static SPI spi(P1_SPI_MOSI, P1_SPI_MISO, P1_SPI_SCK);
ILI9163C display(&spi, P1_SPI_CS, P1_DIO18, P1_PWM1);
static I2C i2c_touchscreen(P1_I2C_SDA, P1_I2C_SCL);
static SX8650IWLTRT sx8650iwltrt(&i2c_touchscreen, P1_DIO5, &event_queue);
static InterruptIn button(BUTTON1);
static DigitalOut led1(LED1);
// Create flash IAP block device
BlockDevice *bd = new FlashIAPBlockDevice(FLASHIAP_ADDRESS_H753ZI, FLASHIAP_SIZE);
/* Driver Touchscreen */
#define BMA280_SWITCHED_TIME 5ms

// Variables
SX8650IWLTRT::coefficient calibration_setting;

void draw_cross(uint8_t x, uint8_t y)
{
    display.clearScreen(0);

    static uint16_t buf[21 * 21] = { 0 };

    display.setAddr(x - 10, y - 10, x + 10, y + 10);

    for (int i = 0; i < 21; i++) {
        if (i == 10) {
            for (int j = 0; j < 21; j++) {
                buf[i * 21 + j] = 0xFFFF;
            }
        } else {
            buf[i * 21 + 10] = 0xFFFF;
        }
    }

    display.write_data_16(buf, 21 * 21);
}

void draw(uint16_t x, uint16_t y)
{
    uint16_t buf = 0xFFFF;
    display.setAddr(x, y, x, y);
    display.write_data_16(&buf, 1);
}

void read_coordinates(uint16_t x, uint16_t y)
{
    printf("-----------------\n\n");
    printf("X : %u | Y : %u \n", x, y);
}

void read_pressure(uint16_t z1, uint16_t z2)
{
    printf("-----------------\n\n");
    printf("Z1 : %u | Z2 : %u \n\n", z1, z2);
}

void clear_settings(SX8650IWLTRT::coefficient *settings)
{
    memset(settings->param, 0, sizeof(settings->param));
}

int read_configuration(SX8650IWLTRT::coefficient *coef)
{
    if (bd->read(coef, 0, sizeof(coef->param)) != 0) {
        return -1;
    }

    uint32_t calculated_crc = 0;
    MbedCRC<POLY_32BIT_ANSI, 32> ct;
    ct.compute(coef->param, sizeof(coef->param) - 4, &calculated_crc);

    if (calculated_crc == coef->crc) {
        return 0;
    }
    return -2;
}

int write_configuration(SX8650IWLTRT::coefficient *coef)
{
    uint32_t calculated_crc = 0;
    MbedCRC<POLY_32BIT_ANSI, 32> ct;
    ct.compute(coef->param, sizeof(coef->param) - 4, &calculated_crc);

    coef->crc = calculated_crc;

    if (bd->erase(0, bd->get_erase_size()) != 0) {
        return -1;
    }

    if (bd->program(coef->param, 0, 32) != 0) {
        return -2;
    }

    if (bd->sync() != 0) {
        return -3;
    }

    return 0;
}

void application_setup(void)
{

    // Initialize the flash IAP block device and print the memory layout
    bd->init();
    printf("Flash block device size: %llu\n", bd->size());
    printf("Flash block device read size: %llu\n", bd->get_read_size());
    printf("Flash block device program size: %llu\n", bd->get_program_size());
    printf("Flash block device erase size: %llu\n", bd->get_erase_size());
    // read configuration
    if (read_configuration(&calibration_setting) != 0) {
        // empty memory, write the default settings
        clear_settings(&calibration_setting);
        printf("First initialization\n");
        lcd_calibration();
        calibration_setting = sx8650iwltrt.get_calibration();
        printf("calib %f %f %f %f %f %f\n",
                calibration_setting.ax,
                calibration_setting.bx,
                calibration_setting.x_off,
                calibration_setting.ay,
                calibration_setting.by,
                calibration_setting.y_off);
        // flash empty, write the default values
        if (write_configuration(&calibration_setting) != 0) {
            printf("[Settings] error flash settings");
            return;
        }
    }
    sx8650iwltrt.set_calibration(calibration_setting.ax,
            calibration_setting.bx,
            calibration_setting.x_off,
            calibration_setting.ay,
            calibration_setting.by,
            calibration_setting.y_off);
}

void lcd_calibration()
{
    printf("\nLCD Calibration ....\n");
    display.clearScreen(0);
    sx8650iwltrt.calibrate(draw_cross);
    printf("Calibration Done\n");
}

void button_handler()
{
    event_queue_button.call(lcd_calibration);
}

void lcd_init()
{
    spi.frequency(24000000);
    display.init();
    printf("\nLCD Initialized\n\n");
    display.clearScreen(0);
    /*Initialize touchscreen component*/
    ThisThread::sleep_for(SX8650_SWITCHED_TIME);
    sx8650iwltrt.soft_reset();
    sx8650iwltrt.set_mode(SX8650IWLTRT::Mode::PenTrg);
    sx8650iwltrt.set_rate(SX8650IWLTRT::Rate::RATE_200_cps);
    sx8650iwltrt.set_powdly(SX8650IWLTRT::Time::DLY_2_2US);
}

int main()
{
    printf("\n-----Start App-----\n");
    lcd_init();
    button.rise(&button_handler);

    // sx8650iwltrt.enable_pressures_measurement();
    sx8650iwltrt.enable_coordinates_measurement();

    thread.start(callback(&event_queue, &EventQueue::dispatch_forever));
    thread_button.start(callback(&event_queue_button, &EventQueue::dispatch_forever));
    application_setup();
    sx8650iwltrt.attach_coordinates_measurement(draw);

    while (true) {
        ThisThread::sleep_for(500ms);
        led1= !led1;
    }
}