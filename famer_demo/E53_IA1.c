/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_i2c.h"
#include "iot_i2c_ex.h"
#include "E53_IA1.h"

/***************************************************************
 * 函数名称: E53IA1IoInit
 * 说    明: E53_IA1 GPIO初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
static void E53IA1IoInit(void)
{
    IoTGpioInit(E53_IA1_MOTOR_GPIO);
    IoSetFunc(E53_IA1_MOTOR_GPIO, WIFI_IOT_IO_FUNC_GPIO_8_GPIO);
    IoTGpioSetDir(E53_IA1_MOTOR_GPIO, IOT_GPIO_DIR_OUT); // 设置GPIO_8为输出模式

    IoTGpioInit(E53_IA1_LIGHT_GPIO);
    IoSetFunc(E53_IA1_LIGHT_GPIO, WIFI_IOT_IO_FUNC_GPIO_14_GPIO);
    IoTGpioSetDir(E53_IA1_LIGHT_GPIO, IOT_GPIO_DIR_OUT); // 设置GPIO_14为输出模式

    IoTGpioInit(E53_IA1_I2C1_SDA_GPIO);
    IoSetFunc(E53_IA1_I2C1_SDA_GPIO, WIFI_IOT_IO_FUNC_GPIO_0_I2C1_SDA); // GPIO_0复用为I2C1_SDA
    IoTGpioInit(E53_IA1_I2C1_SCL_GPIO);
    IoSetFunc(E53_IA1_I2C1_SCL_GPIO, WIFI_IOT_IO_FUNC_GPIO_1_I2C1_SCL); // GPIO_1复用为I2C1_SCL
    IoTI2cInit(WIFI_IOT_I2C_IDX_1, WIFI_IOT_I2C_BAUDRATE);
}

/***************************************************************
 * 函数名称: Init_BH1750
 * 说    明: 写命令初始化BH1750
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
static int InitBH1750(void)
{
    int ret;
    uint8_t send_data[1] = { 0x01 };
    ret = IoTI2cWrite(WIFI_IOT_I2C_IDX_1, (BH1750_ADDR << 1) | 0x00, send_data, 1);
    if (ret != 0) {
        printf("===== Error: I2C write ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

/***************************************************************
 * 函数名称: StartBH1750
 * 说    明: 启动BH1750
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
static int StartBH1750(void)
{
    int ret;
    uint8_t send_data[1] = { 0x10 };
    ret = IoTI2cWrite(WIFI_IOT_I2C_IDX_1, (BH1750_ADDR << 1) | 0x00, send_data, 1);
    if (ret != 0) {
        printf("===== Error: I2C write ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

/***************************************************************
 * 函数名称: SHT30Reset
 * 说    明: SHT30复位
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
static int SHT30Reset(void)
{
    int ret;
    uint8_t send_data[BH1750_SEND_DATA_LEN] = { 0x30, 0xA2 };
    ret = IoTI2cWrite(WIFI_IOT_I2C_IDX_1, (SHT30_ADDR << 1) | 0x00, send_data, sizeof(send_data));
    if (ret != 0) {
        printf("===== Error: I2C write ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

/***************************************************************
 * 函数名称: InitSHT30
 * 说    明: 初始化SHT30，设置测量周期
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
static int InitSHT30(void)
{
    int ret;
    uint8_t send_data[BH1750_SEND_DATA_LEN] = { 0x22, 0x36 };
    ret = IoTI2cWrite(WIFI_IOT_I2C_IDX_1, (SHT30_ADDR << 1) | 0x00, send_data, sizeof(send_data));
    if (ret != 0) {
        printf("===== Error: I2C write ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

/***************************************************************
* 函数名称: SHT3xCheckCrc
* 说    明: 检查数据正确性
* 参    数: data：读取到的数据
                        nbrOfBytes：需要校验的数量
                        checksum：读取到的校对比验值
* 返 回 值: 校验结果，0-成功		1-失败
***************************************************************/
static uint8_t SHT3xCheckCrc(uint8_t data[], uint8_t nbrOfBytes, uint8_t checksum)
{
    uint8_t crc = 0xFF;
    uint8_t bit = 0;
    uint8_t byteCtr;
    const int16_t POLYNOMIAL = 0x131;
    // calculates 8-Bit checksum with given polynomial
    for (byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr) {
        crc ^= (data[byteCtr]);
        for (bit = DATA_WIDTH_8_BIT; bit > 0; --bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }

    if (crc != checksum)
        return 1;
    else
        return 0;
}

/***************************************************************
 * 函数名称: SHT3xCalcTemperatureC
 * 说    明: 温度计算
 * 参    数: u16sT：读取到的温度原始数据
 * 返 回 值: 计算后的温度数据
 ***************************************************************/
static float SHT3xCalcTemperatureC(uint16_t u16sT)
{
    float temperatureC = 0; // variable for result
    u16sT &= ~0x0003;       // clear bits [1..0] (status bits)
    // -- calculate temperature [℃] --
    temperatureC = (SHT30_CONSTANT_1 * (float)u16sT / 0xffff - SHT30_CONSTANT_2); // T = -45 + 175 * rawValue / (2^16-1)
    return temperatureC;
}

/***************************************************************
 * 函数名称: SHT3xCalcRH
 * 说    明: 湿度计算
 * 参    数: u16sRH：读取到的湿度原始数据
 * 返 回 值: 计算后的湿度数据
 ***************************************************************/
static float SHT3xCalcRH(uint16_t u16sRH)
{
    float humidityRH = 0; // variable for result
    u16sRH &= ~0x0003;    // clear bits [1..0] (status bits)
    // -- calculate relative humidity [%RH] --
    humidityRH = (SHT30_CONSTANT_3 * (float)u16sRH / 0xffff); // RH = rawValue / (2^16-1) * 10
    return humidityRH;
}

/***************************************************************
 * 函数名称: E53IA1Init
 * 说    明: 初始化E53_IA1
 * 参    数: 无
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
int E53IA1Init(void)
{
    int ret;
    E53IA1IoInit();
    ret = InitBH1750();
    if (ret != 0) {
        printf("Init BH1750 failed!\r\n");
        return -1;
    }
    ret = InitSHT30();
    if (ret != 0) {
        printf("Init SHT30 failed!\r\n");
        return -1;
    }
}

/***************************************************************
 * 函数名称: E53IA1ReadData
 * 说    明: 测量光照强度、温度、湿度
 * 参    数: ReadData,光照强度、温度、湿度数据
 * 返 回 值: 0 成功; -1 失败
 ***************************************************************/
int E53IA1ReadData(E53IA1Data *ReadData)
{
    int ret;
    ret = StartBH1750(); // 启动传感器采集数据
    if (ret != 0) {
        printf("Start BH1750 failed!\r\n");
        return -1;
    }
    usleep(BH1750_READ_DELAY_US);
    uint8_t recv_data[BH1750_RECV_DATA_LEN] = { 0 };
    ret = IoTI2cRead(WIFI_IOT_I2C_IDX_1, (BH1750_ADDR << 1) | 0x01, recv_data, sizeof(recv_data)); // 读取传感器数据
    if (ret != 0) {
        return -1;
    }
    ReadData->Lux = (float)(((recv_data[0] << DATA_WIDTH_8_BIT) + recv_data[1]) / BH1750_COEFFICIENT_LUX);

    uint8_t data[CHECK_DATA_BUTT]; // data array for checksum verification
    uint16_t dat, tmp;
    uint8_t SHT3X_Data_Buffer[SHT3X_DATA_BUTT]; // byte 0,1 is temperature byte 4,5 is humidity

    IotI2cData sht30_i2c_data = { 0 };
    uint8_t send_data[SHT30_SEND_DATA_LEN] = { 0xE0, 0x00 };
    sht30_i2c_data.sendBuf = send_data;
    sht30_i2c_data.sendLen = sizeof(send_data);
    sht30_i2c_data.receiveBuf = SHT3X_Data_Buffer;
    sht30_i2c_data.receiveLen = sizeof(SHT3X_Data_Buffer);
    ret = IoTI2cWriteread(WIFI_IOT_I2C_IDX_1, (SHT30_ADDR << 1) | 0x00, &sht30_i2c_data); // Read bh1750 sensor data
    if (ret != 0) {
        return -1;
    }
    /* check tem */
    data[DATA_LSB] = SHT3X_Data_Buffer[TEMP_LSB];
    data[DATA_MSB] = SHT3X_Data_Buffer[TEMP_MSB];
    data[DATA_CHECK] = SHT3X_Data_Buffer[TEMP_HSB];

    tmp = SHT3xCheckCrc(data, SHT30_TEMP_DATA_LEN, data[DATA_CHECK]);
    /* value is ture */
    if (!tmp) {
        dat = ((uint16_t)data[0] << DATA_WIDTH_8_BIT) | data[1];
        ReadData->Temperature = SHT3xCalcTemperatureC(dat);
    }

    /* check humidity */
    data[DATA_LSB] = SHT3X_Data_Buffer[HUM_LSB];
    data[DATA_MSB] = SHT3X_Data_Buffer[HUM_MSB];
    data[DATA_CHECK] = SHT3X_Data_Buffer[HUM_HSB];
    /* value is ture */
    tmp = SHT3xCheckCrc(data, SHT30_HUM_DATA_LEN, data[DATA_CHECK]);
    if (!tmp) {
        dat = ((uint16_t)data[0] << DATA_WIDTH_8_BIT) | data[1];
        ReadData->Humidity = SHT3xCalcRH(dat);
    }
    return 0;
}

/***************************************************************
 * 函数名称: LightStatusSet
 * 说    明: 灯状态设置
 * 参    数: status,ENUM枚举的数据
 *									OFF,关
 *									ON,开
 * 返 回 值: 无
 ***************************************************************/
void LightStatusSet(E53IA1Status status)
{
    if (status == ON) {
        IoTGpioSetOutputVal(E53_IA1_LIGHT_GPIO, 1); // 设置GPIO_14输出高电平点亮灯
    }

    if (status == OFF) {
        IoTGpioSetOutputVal(E53_IA1_LIGHT_GPIO, 0); // 设置GPIO_14输出低电平关闭灯
    }
}

/***************************************************************
 * 函数名称: MotorStatusSet
 * 说    明: 电机状态设置
 * 参    数: status,ENUM枚举的数据
 *									OFF,关
 *									ON,开
 * 返 回 值: 无
 ***************************************************************/
void MotorStatusSet(E53IA1Status status)
{
    if (status == ON) {
        IoTGpioSetOutputVal(E53_IA1_MOTOR_GPIO, 1); // 设置GPIO_8输出高电平打开电机
    }

    if (status == OFF) {
        IoTGpioSetOutputVal(E53_IA1_MOTOR_GPIO, 0); // 设置GPIO_8输出低电平关闭电机
    }
}
