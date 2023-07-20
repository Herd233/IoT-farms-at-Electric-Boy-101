#include <stdio.h>
#include <stdlib.h>

#include <iot_gpio.h>
#include "hi_io.h"
#include "iot_gpio_ex.h"
#include "iot_watchdog.h"
#include "hi_time.h"
#include "sg92r.h"

#define  COUNT   10
#define  FREQ_TIME    20000

void Engine1SetAngle(unsigned int duty)
{
    unsigned int time = FREQ_TIME;

    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_10, IOT_GPIO_VALUE1);
    hi_udelay(duty);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_10, IOT_GPIO_VALUE0);
    hi_udelay(time - duty);
}
void Engine2SetAngle(unsigned int duty)
{
    unsigned int time = FREQ_TIME;

    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_11, IOT_GPIO_VALUE1);
    hi_udelay(duty);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_11, IOT_GPIO_VALUE0);
    hi_udelay(time - duty);
}
void Engine3SetAngle(unsigned int duty)
{
    unsigned int time = FREQ_TIME;

    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_12, IOT_GPIO_VALUE1);
    hi_udelay(duty);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_12, IOT_GPIO_VALUE0);
    hi_udelay(time - duty);
}


/* The steering gear is centered
 * 1、依据角度与脉冲的关系，设置高电平时间为1500微秒
 * 2、不断地发送信号，控制舵机居中
*/
void Engine1RegressMiddle(void)
{
    unsigned int angle = 1500;
    for (int i = 0; i < COUNT; i++) {
        Engine1SetAngle(angle);
    }
}

/* The steering gear is centered
 * 1、依据角度与脉冲的关系，设置高电平时间为1500微秒
 * 2、不断地发送信号，控制舵机居中
*/
void Engine2RegressMiddle(void)
{
    unsigned int angle = 1500;
    for (int i = 0; i < COUNT; i++) {
        Engine2SetAngle(angle);
    }
}

/* The steering gear is centered
 * 1、依据角度与脉冲的关系，设置高电平时间为1500微秒
 * 2、不断地发送信号，控制舵机居中
*/
void Engine3RegressMiddle(void)
{
    unsigned int angle = 1500;
    for (int i = 0; i < COUNT; i++) {
        Engine3SetAngle(angle);
    }
}

/* Turn 90 degrees to the right of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为500微秒
 * 2、不断地发送信号，控制舵机向右旋转90度
*/
/*  Steering gear turn right */
void Engine1TurnRight_90(void)
{
    unsigned int angle = 500;
    for (int i = 0; i < COUNT; i++) {
        Engine1SetAngle(angle);
    }
}
/* Turn 90 degrees to the right of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为1000微秒
 * 2、不断地发送信号，控制舵机向右旋转45度
*/
/*  Steering gear turn right */
void Engine1TurnRight_45(void)
{
    unsigned int angle = 1000;
    for (int i = 0; i < COUNT; i++) {
        Engine1SetAngle(angle);
    }
}

/* Turn 90 degrees to the left of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为2500微秒
 * 2、不断地发送信号，控制舵机向左旋转90度
*/
/* Steering gear turn left */
void Engine1TurnLeft_90(void)
{
    unsigned int angle = 2500;
    for (int i = 0; i < COUNT; i++) {
        Engine1SetAngle(angle);
    }
}
/* Turn 90 degrees to the left of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为2000微秒
 * 2、不断地发送信号，控制舵机向左旋转45度
*/
/* Steering gear turn left */
void Engine1TurnLeft_45(void)
{
    unsigned int angle = 2000;
    for (int i = 0; i < COUNT; i++) {
        Engine1SetAngle(angle);
    }
}

/* Turn 90 degrees to the right of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为500微秒
 * 2、不断地发送信号，控制舵机向右旋转90度
*/
/*  Steering gear turn right */
void Engine2TurnRight_90(void)
{
    unsigned int angle = 500;
    for (int i = 0; i < COUNT; i++) {
        Engine2SetAngle(angle);
    }
}
/* Turn 90 degrees to the right of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为1000微秒
 * 2、不断地发送信号，控制舵机向右旋转45度
*/
/*  Steering gear turn right */
void Engine2TurnRight_45(void)
{
    unsigned int angle = 1000;
    for (int i = 0; i < COUNT; i++) {
        Engine2SetAngle(angle);
    }
}

/* Turn 90 degrees to the left of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为2500微秒
 * 2、不断地发送信号，控制舵机向左旋转90度
*/
/* Steering gear turn left */
void Engine2TurnLeft_90(void)
{
    unsigned int angle = 2500;
    for (int i = 0; i < COUNT; i++) {
        Engine2SetAngle(angle);
    }
}
/* Turn 90 degrees to the left of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为2000微秒
 * 2、不断地发送信号，控制舵机向左旋转45度
*/
/* Steering gear turn left */
void Engine2TurnLeft_45(void)
{
    unsigned int angle = 2000;
    for (int i = 0; i < COUNT; i++) {
        Engine2SetAngle(angle);
    }
}

/* Turn 90 degrees to the right of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为500微秒
 * 2、不断地发送信号，控制舵机向右旋转90度
*/
/*  Steering gear turn right */
void Engine3TurnRight_90(void)
{
    unsigned int angle = 500;
    for (int i = 0; i < COUNT; i++) {
        Engine3SetAngle(angle);
    }
}
/* Turn 90 degrees to the right of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为1000微秒
 * 2、不断地发送信号，控制舵机向右旋转45度
*/
/*  Steering gear turn right */
void Engine3TurnRight_45(void)
{
    unsigned int angle = 1000;
    for (int i = 0; i < COUNT; i++) {
        Engine3SetAngle(angle);
    }
}

/* Turn 90 degrees to the left of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为2500微秒
 * 2、不断地发送信号，控制舵机向左旋转90度
*/
/* Steering gear turn left */
void Engine3TurnLeft_90(void)
{
    unsigned int angle = 2500;
    for (int i = 0; i < COUNT; i++) {
        Engine3SetAngle(angle);
    }
}
/* Turn 90 degrees to the left of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为2000微秒
 * 2、不断地发送信号，控制舵机向左旋转45度
*/
/* Steering gear turn left */
void Engine3TurnLeft_45(void)
{
    unsigned int angle = 2000;
    for (int i = 0; i < COUNT; i++) {
        Engine3SetAngle(angle);
    }
}

void SG92RInit(void)
{
    IoTGpioInit(IOT_IO_NAME_GPIO_10);
    IoSetFunc(IOT_IO_NAME_GPIO_10, IOT_IO_FUNC_GPIO_10_GPIO);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_10, IOT_GPIO_DIR_OUT);
    IoTGpioInit(IOT_IO_NAME_GPIO_11);
    IoSetFunc(IOT_IO_NAME_GPIO_11, IOT_IO_FUNC_GPIO_11_GPIO);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_11, IOT_GPIO_DIR_OUT);
    IoTGpioInit(IOT_IO_NAME_GPIO_12);
    IoSetFunc(IOT_IO_NAME_GPIO_12, IOT_IO_FUNC_GPIO_12_GPIO);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_12, IOT_GPIO_DIR_OUT);
}