/*
 * Copyright (c) 2021, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include "sx8650iwltrt.h"

using namespace sixtron;

namespace {
#define BMA280_SWITCHED_TIME 5ms
#define PERIOD_MS 3000ms
}

/* Definitions PIN */
static SX8650IWLTRT sx8650iwltrt(I2C1_SDA, I2C1_SCL);
static DigitalOut led1(LED1);


// void read_position(){
//     //dans i2CRegChanMsk x_conv et y_conv a 1
//     char tmp = static_cast<char>(RegChanMskAddress::CONV_X) + static_cast<char>(RegChanMskAddress::CONV_Y);
//     //write in i2CRegChanMsk register
//     sx8650iwltrt.i2c_set_register(RegisterAddress::I2CRegChanMsk,tmp);
//     //read channel
//     sx8650iwltrt.

// }



int main()
{
    printf("--------------------------------------\n\n");
    printf("SX8650IWLTRT library example\n\n");
    ThisThread::sleep_for(BMA280_SWITCHED_TIME);
    sx8650iwltrt.soft_reset();
    printf("Soft Reset done\n\n");
    sx8650iwltrt.set_mode(Mode::ManAuto);
    printf("SX8650IWLTRT in Automatic mode\n\n");
    sx8650iwltrt.set_rate(Rate::RATE_10_cps);
    printf("Rate Component : %d \n\n",static_cast<int>(sx8650iwltrt.rate()));
    sx8650iwltrt.set_rate(Rate::RATE_100_cps);
    printf("Rate Component : %u \n\n",static_cast<uint8_t>(sx8650iwltrt.rate()));
    while(1){
        printf("-----------------\n\n");
        printf("Enter in loop \n\n");
        sx8650iwltrt.select_channel(ChannelAddress::CH_X);
        sx8650iwltrt.convert_channel(ChannelAddress::CH_X);
        char data = 0;
        sx8650iwltrt.i2c_read_channel(ChannelAddress::CH_X,&data);
        printf("Position X : %d \n\n",int(data));

        led1 = !led1;
        ThisThread::sleep_for(PERIOD_MS);

    }
}
