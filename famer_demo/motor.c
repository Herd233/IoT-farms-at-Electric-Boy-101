#include <stdio.h>
#include <unistd.h>

#include "iot_gpio_ex.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_pwm.h"
#include "hi_io.h"
#include "iot_errno.h"
#include "motor.h"

#define IOT_DUTY 50
#define IOT_FREQ 4000
#define IOT_PWM_PORT_PWM0  0
void motor_Init(void)
{
    IoTGpioInit(IOT_IO_NAME_GPIO_9);
    IoSetFunc(IOT_IO_NAME_GPIO_9,IOT_IO_FUNC_GPIO_9_PWM0_OUT);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_9,IOT_GPIO_DIR_OUT);
    IoTPwmInit(IOT_PWM_PORT_PWM0);
    IoTGpioInit(IOT_IO_NAME_GPIO_8);
    IoSetFunc(IOT_IO_NAME_GPIO_8,IOT_IO_FUNC_GPIO_8_GPIO);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_8,IOT_GPIO_DIR_OUT);
    IoTGpioInit(IOT_IO_NAME_GPIO_7);
    IoSetFunc(IOT_IO_NAME_GPIO_7,IOT_IO_FUNC_GPIO_7_GPIO);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_7,IOT_GPIO_DIR_OUT);
    motor_stop();
}

void motor_forward(void)
{
    IoTPwmStart(IOT_PWM_PORT_PWM0,IOT_DUTY,IOT_FREQ);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_8,IOT_GPIO_VALUE0);
    TaskMsleep(1000);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_7,IOT_GPIO_VALUE1);
    TaskMsleep(1000);
}

void motor_reversal(void)
{
    IoTPwmStart(IOT_PWM_PORT_PWM0,IOT_DUTY,IOT_FREQ);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_8,IOT_GPIO_VALUE1);
    TaskMsleep(1000);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_7,IOT_GPIO_VALUE0);
    TaskMsleep(1000);
}

void motor_stop(void)
{
    IoTPwmStop(IOT_PWM_PORT_PWM0);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_8,IOT_GPIO_VALUE0);
    TaskMsleep(1000);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_7,IOT_GPIO_VALUE0);
    TaskMsleep(1000);
}