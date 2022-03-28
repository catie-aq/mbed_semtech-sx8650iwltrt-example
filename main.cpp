/* mbed Microcontroller Library
 * Copyright (c) 2020 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "BlockDevice.h"
#include "FATFileSystem.h"
#include "FlashIAPBlockDevice.h"
#include "ili9163c.h"
#include "mbed.h"
#include "sx8650iwltrt.h"
#include <errno.h>

using namespace sixtron;

#define FLASH_ENABLE 1 // state of led flash during the capture
#define FLASHIAP_ADDRESS 0x08055000 // 0x0800000 + 350 kB
#define FLASHIAP_SIZE 0x70000 // 0x61A80    // 460 kB

#define VALUE_ACCURACY_X 7 * 128 / 100
#define VALUE_ACCURACY_Y 7 * 180 / 100

/* Driver Touchscreen */
#define BMA280_SWITCHED_TIME 5ms

// Protoypes
void application_setup(void);
void application(void);
void draw_cross(uint8_t x, uint8_t y);
void draw(uint16_t x, uint16_t y);
void read_coordinates(uint16_t x, uint16_t y);
void read_pressures(uint16_t z1, uint16_t z2);
void retrieve_calibrate_data(char *bufer);
void save_calibrate_date(char *buffer);

// RTOS
Thread thread;
static EventQueue event_queue;

// Preipherals
static SX8650IWLTRT sx8650iwltrt(I2C1_SDA, I2C1_SCL, &event_queue, I2CAddress::Address2);
static SPI spi(SPI1_MOSI, SPI1_MISO, SPI1_SCK);
ILI9163C display(&spi, SPI1_CS, DIO18, PWM1_OUT);
static DigitalOut led1(LED1);
static InterruptIn button(BUTTON1);

// Create flash IAP block device
BlockDevice *bd = new FlashIAPBlockDevice(FLASHIAP_ADDRESS, FLASHIAP_SIZE);

FATFileSystem fs("fs");

// Variables
bool correct_calibrate(false);

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
    printf("X : %u | Y : %u \n\n", x, y);
}

void read_pressure(uint16_t z1, uint16_t z2)
{
    printf("-----------------\n\n");
    printf("Z1 : %u | Z2 : %u \n\n", z1, z2);
}

void application_setup(void)
{

    // Initialize the flash IAP block device and print the memory layout
    bd->init();
    printf("Flash block device size: %llu\n", bd->size());
    printf("Flash block device read size: %llu\n", bd->get_read_size());
    printf("Flash block device program size: %llu\n", bd->get_program_size());
    printf("Flash block device erase size: %llu\n", bd->get_erase_size());

    printf("Mounting the filesystem... ");
    fflush(stdout);
    int err = fs.mount(bd);

    printf("%s\n", (err ? "Fail :(" : "OK"));
    if (err) {
        // Reformat if we can't mount the filesystem
        // this should only happen on the first boot
        printf("No filesystem found, formatting... ");
        fflush(stdout);
        err = fs.reformat(bd);
        printf("%s\n", (err ? "Fail :(" : "OK"));
        if (err) {
            error("error: %s (%d)\n", strerror(-err), err);
        }
    }
}

void retrieve_calibrate_data(char *buffer)
{
    // Open the file
    printf("Opening \"/fs/calibration.csv\"... ");
    fflush(stdout);
    FILE *f = fopen("/fs/calibration.csv", "r+");
    printf("%s\n", (!f ? "Fail :(" : "OK"));
    if (!f) {
        // Create the file if it doesn't exist
        printf("No file found, creating a new file... ");
        fflush(stdout);
        f = fopen("/fs/calibration.csv", "w+");
        printf("%s\n", (!f ? "Fail :(" : "OK"));
        if (!f) {
            error("error: %s (%d)\n", strerror(errno), -errno);
        }
    }
    double ax, bx, x_off, ay, by, y_off;
    bd->read(buffer, 0, bd->get_erase_size());
    scanf("%f,%f,%f,%f,%f,%f", &ax, &bx, &x_off, &ay, &by, &y_off);
    printf("%s", buffer);
    sx8650iwltrt.set_calibration(ax, bx, x_off, ay, by, y_off);

    fclose(f);

    // Display the root directory
    printf("Opening the root directory... ");
    fflush(stdout);
    DIR *d = opendir("/fs/");
    printf("%s\n", (!d ? "Fail :(" : "OK"));
    if (!d) {
        error("error: %s (%d)\n", strerror(errno), -errno);
    }

    printf("root directory:\n");
    while (true) {
        struct dirent *e = readdir(d);
        if (!e) {
            break;
        }

        printf("    %s\n", e->d_name);
    }
}

void save_calibrate_date(char *buffer)
{
    // Open the file
    printf("Opening \"/fs/calibration.csv\"... ");
    fflush(stdout);
    FILE *f = fopen("/fs/calibration.csv", "r+");
    printf("%s\n", (!f ? "Fail :(" : "OK"));
    if (!f) {
        // Create the file if it doesn't exist
        printf("No file found, creating a new file... ");
        fflush(stdout);
        f = fopen("/fs/calibration.csv", "w+");
        printf("%s\n", (!f ? "Fail :(" : "OK"));
        if (!f) {
            error("error: %s (%d)\n", strerror(errno), -errno);
        }
    }
    sprintf(buffer,
            "%f,%f,%f,%f,%f,%f \n\n",
            sx8650iwltrt._coefficient.ax,
            sx8650iwltrt._coefficient.bx,
            sx8650iwltrt._coefficient.x_off,
            sx8650iwltrt._coefficient.ay,
            sx8650iwltrt._coefficient.by,
            sx8650iwltrt._coefficient.y_off);
    bd->erase(0, bd->get_erase_size());
    bd->program(buffer, 0, bd->get_erase_size());

    fclose(f);

    // Display the root directory
    printf("Opening the root directory... ");
    fflush(stdout);
    DIR *d = opendir("/fs/");
    printf("%s\n", (!d ? "Fail :(" : "OK"));
    if (!d) {
        error("error: %s (%d)\n", strerror(errno), -errno);
    }

    printf("root directory:\n");
    while (true) {
        struct dirent *e = readdir(d);
        if (!e) {
            break;
        }

        printf("    %s\n", e->d_name);
    }
}

int main()
{
    printf("Start App\n\n");

    display.init();
    display.clearScreen(0);
    thread.start(callback(&event_queue, &EventQueue::dispatch_forever));
    /*Initialize touchscreen component*/
    ThisThread::sleep_for(BMA280_SWITCHED_TIME);
    sx8650iwltrt.soft_reset();
    sx8650iwltrt.set_mode(Mode::PenTrg);
    sx8650iwltrt.set_rate(Rate::RATE_200_cps);
    sx8650iwltrt.set_powdly(Time::DLY_2_2US);

    // sx8650iwltrt.enable_pressures_measurement();
    sx8650iwltrt.enable_coordinates_measurement();

    // application setup
    application_setup();
    char *buffer = (char *)malloc(bd->get_erase_size());

    if (sx8650iwltrt._coefficient.ax != 2.00) {
        retrieve_calibrate_data(buffer);
        correct_calibrate = true;
    }
    while (!correct_calibrate) {
        draw_cross(64, 80);
        sx8650iwltrt.attach_coordinates_measurement(read_coordinates);
        ThisThread::sleep_for(2000ms);
        if ((64 + VALUE_ACCURACY_X > sx8650iwltrt._coordinates.x
                    && sx8650iwltrt._coordinates.x > 64 - VALUE_ACCURACY_X)
                && (80 + VALUE_ACCURACY_Y > sx8650iwltrt._coordinates.y
                        && sx8650iwltrt._coordinates.y > 80 - VALUE_ACCURACY_Y)) {
            correct_calibrate = true;
            save_calibrate_date(buffer);
            printf("Calibrate done correctly ! \n\n");
        } else {
            sx8650iwltrt.calibrate(draw_cross);
            printf("Calibrate again ! \n\n");
        }
        display.clearScreen(0);
    }

    sx8650iwltrt.attach_coordinates_measurement(draw);
    sx8650iwltrt.attach_pressures_measurement(read_pressure);
    
    bd->read(buffer, 0, bd->get_erase_size());
    printf("%s", buffer);

    // Deinitialize the device
    bd->deinit();

    while (true) {
        led1 = !led1;
        ThisThread::sleep_for(100ms);
    }
}