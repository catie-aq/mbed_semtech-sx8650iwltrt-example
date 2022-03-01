/* mbed Microcontroller Library
 * Copyright (c) 2020 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ili9163c.h"
#include "mbed.h"
#include "sx8650iwltrt.h"

using namespace sixtron;

static SPI spi(SPI1_MOSI, SPI1_MISO, SPI1_SCK);
ILI9163C display(&spi, SPI1_CS, DIO18, PWM1_OUT);
static DigitalOut led1(LED1);
Thread thread;
static EventQueue event_queue;

/* Driver Touchscreen */
#define BMA280_SWITCHED_TIME 5ms
static SX8650IWLTRT sx8650iwltrt(I2C1_SDA, I2C1_SCL,&event_queue, I2CAddress::Address2);

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
    uint16_t buf = 0x1234;
    display.setAddr(x, y, x, y);
    display.write_data_16(&buf, 1);
}

void read_coordinates(uint16_t x, uint16_t y){
    printf("-----------------\n\n");
    printf("X : %u | Y : %u \n\n",x,y);
}



int main()
{
    printf("Start App\n");
    display.init();
    thread.start(callback(&event_queue, &EventQueue::dispatch_forever));        
    /*Initialize touchscreen component*/
    ThisThread::sleep_for(BMA280_SWITCHED_TIME);
    sx8650iwltrt.soft_reset();
    sx8650iwltrt.set_mode(Mode::PenTrg);

    sx8650iwltrt.calibrate(draw_cross);
    sx8650iwltrt.attach(draw);
    
    while (true) {
        led1 = !led1;
        ThisThread::sleep_for(100ms);
    }
}
