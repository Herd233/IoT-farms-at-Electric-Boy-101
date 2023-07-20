/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SGP30.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_i2c.h"
#include "iot_pwm.h"
#include "cmsis_os2.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CAL_PPM 20 //校准环境中PPM值
#define RL 1       // RL阻值
#define IOT_IO_NAME_GPIO_9 9
#define IOT_PWM_PORT_PWM0 1
#define IOT_IO_FUNC_GPIO_9_PWM0_OUT 6
#define IOT_IO_NAME_GPIO_0           0
#define IOT_IO_FUNC_GPIO_0_I2C1_SDA  6
#define IOT_IO_NAME_GPIO_1           1
#define IOT_IO_FUNC_GPIO_1_I2C1_SCL  6
#define IOT_SGP30_I2C_IDX_1           1
#define I2C_IDX_BAUDRATE                  (400000)
#define SGP30_I2C_ADDRESS                 0xb0

/***************************************************************
 * 函数名称: SGP30Init
 * 说    明: 初始化甲醛传感器
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void SGP30Init(void)
{
    unsigned result = 0;
    //IoTGpioInit(IOT_IO_NAME_GPIO_9);                                      // 初始化GPIO_9
    //IoSetFunc(IOT_IO_NAME_GPIO_9, IOT_IO_FUNC_GPIO_9_PWM0_OUT); // 设置GPIO_9引脚复用功能为PWM
    //IoTGpioSetDir(IOT_IO_NAME_GPIO_9, IOT_GPIO_DIR_OUT);                  // 设置GPIO_9引脚为输出模式
    //IoTPwmInit(IOT_PWM_PORT_PWM0);                                        // 初始化PWM0端口

    char sendInitCmd [2] = {0x20, 0x03};
    result = IoTI2cWrite(IOT_SGP30_I2C_IDX_1, SGP30_I2C_ADDRESS, sendInitCmd, 2);
    printf("SGP30Init result %d\n", result);
}

static void I2cBusInit(void)
{
    unsigned int result = 0;
    IoTGpioInit(IOT_IO_NAME_GPIO_0);
    IoSetFunc(IOT_IO_NAME_GPIO_0, IOT_IO_FUNC_GPIO_0_I2C1_SDA); // Set up the gpio funcion as i2c bus
    //IoSetPull(IOT_IO_NAME_GPIO_0,IOT_IO_PULL_UP);
    IoTGpioInit(IOT_IO_NAME_GPIO_1);
    IoSetFunc(IOT_IO_NAME_GPIO_1, IOT_IO_FUNC_GPIO_1_I2C1_SCL);
    //IoSetPull(IOT_IO_NAME_GPIO_1,IOT_IO_PULL_UP);
    result = IoTI2cInit(IOT_SGP30_I2C_IDX_1, I2C_IDX_BAUDRATE); // Rate: 100kbps
    

    printf("I2C1 bus init result :%d\n", result);
}

/**
 *  @brief initialize the SGP30 extended board, the sensor could work as espected
 */
int SGP30_Init(void)
{
    I2cBusInit();
    SGP30Init();
    return 0;
}

/**
 * @brief Get the v value
 * @param TVOCData used to storage the value and could not be NULL
 * @return Returns 0 success while others failed
 */
int SGP30_GetCO2(void)
{
    float CO2Data;
    float TVOCData;
    unsigned char data[6] = {0};
    unsigned int result = 0;
    int retry_counts = 3;

    char sendReadCmd [2] = {0x20, 0x08};
    result = IoTI2cWrite(IOT_SGP30_I2C_IDX_1, SGP30_I2C_ADDRESS, sendReadCmd, 2);
    if (result != 0) {
        printf("SGP30_GetVOC sendReadCmd failed ! result = %d\n", result);
        return -1;
    }
    
    osDelay(10);
    result = IoTI2cRead(IOT_SGP30_I2C_IDX_1, SGP30_I2C_ADDRESS, data, 6);
    if (result != 0) {
        printf("SGP30_GetVOC IoTI2cRead failed ! result = %d\n", result);
        return -1;
    }
    
    CO2Data = (uint16_t)data[0] << 8 | (uint16_t)data[1];
    TVOCData = (uint16_t)data[3] << 8 | (uint16_t)data[4];
    return CO2Data;
}



