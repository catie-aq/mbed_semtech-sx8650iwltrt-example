/*
 * Copyright (c) 2021, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include "sx8650iwltrt.h"
// #include "swo.h"

using namespace sixtron;

namespace {
#define BMA280_SWITCHED_TIME 5ms
#define PERIOD_MS 500ms
}

/* Definitions PIN */
static SX8650IWLTRT sx8650iwltrt(I2C1_SDA, I2C1_SCL);
static DigitalOut led1(LED1);
InterruptIn nirq(DIO5);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t;



void nirq_fall(){
    sx8650iwltrt.read_channel();
    printf("-----------------\n\n");
    printf("X : %u | Y : %u \n\n",sx8650iwltrt.coordinates.x,sx8650iwltrt.coordinates.y);
    printf("-----------------\n\n");
    led1 = !led1;   
}


int main()
{
    t.start(callback(&queue, &EventQueue::dispatch_forever));
    

    printf("--------------------------------------\n\n");
    printf("SX8650IWLTRT library example\n\n");
    
    ThisThread::sleep_for(BMA280_SWITCHED_TIME);
    sx8650iwltrt.soft_reset();
    
    printf("Soft Reset done\n\n");
    printf("Default Rate Component : %u cps\n\n",static_cast<uint8_t>(sx8650iwltrt.rate()));
    sx8650iwltrt.set_rate(Rate::RATE_20_cps);
    sx8650iwltrt.set_condirq(RegCtrl1Address::CONDIRQ);
    sx8650iwltrt.set_mode(Mode::PenTrg);
    
    printf("SX8650IWLTRT in Automatic mode\n\n");
    printf("Rate Component : %u cps\n\n",static_cast<uint8_t>(sx8650iwltrt.rate()));
    printf("Status CONVIRQ Interrupt : %u cps\n\n",static_cast<uint8_t>(sx8650iwltrt.convirq()));
    
    nirq.fall(queue.event(nirq_fall));
    return 0;
}
