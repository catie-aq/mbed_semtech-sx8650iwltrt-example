/*
 * Copyright (c) 2021, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include "sx8650iwltrt.h"
#include "swo.h"

using namespace sixtron;

namespace {
#define BMA280_SWITCHED_TIME 5ms
#define PERIOD_MS 500ms
}

/* Definitions PIN */
static SX8650IWLTRT sx8650iwltrt(I2C1_SDA, I2C1_SCL);
static DigitalOut led1(LED1);
SWO swo;


int main()
{
    printf("--------------------------------------\n\n");
    printf("SX8650IWLTRT library example\n\n");
    ThisThread::sleep_for(BMA280_SWITCHED_TIME);
    sx8650iwltrt.soft_reset();
    printf("Soft Reset done\n\n");
    printf("Default Rate Component : %u cps\n\n",static_cast<uint8_t>(sx8650iwltrt.rate()));
    sx8650iwltrt.set_rate(Rate::RATE_2K_cps);
    sx8650iwltrt.set_condirq(RegCtrl1Address::CONDIRQ);
    sx8650iwltrt.set_mode(Mode::ManAuto);
    printf("SX8650IWLTRT in Automatic mode\n\n");
    printf("Rate Component : %u cps\n\n",static_cast<uint8_t>(sx8650iwltrt.rate()));
    printf("Condriq status : %u\n\n",static_cast<uint8_t>(sx8650iwltrt.condirq()));
    

    while(1){
        printf("-----------------\n\n");
        printf("Enter in loop \n\n");
        led1 = !led1;
        printf("Data read from channel : %u \n\n",sx8650iwltrt.read_channel());
        printf("Position X : %u \n\n",sx8650iwltrt.read_channel_data());

        ThisThread::sleep_for(PERIOD_MS);

    }
    return 0;
}
