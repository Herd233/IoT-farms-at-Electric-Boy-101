/*
 * Copyright (C) 2021 HiHope Open Source Organization .
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
 *
 * limitations under the License.
 */

#include "aht20.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "hi_i2c.h"
#include "hi_errno.h"

#define AHT20_I2C_IDX HI_I2C_IDX_1

#define AHT20_STARTUP_TIME     (20 * 1000) // 上电启动时间
#define AHT20_CALIBRATION_TIME (40 * 1000) // 初始化（校准）时间
#define AHT20_MEASURE_TIME     (75 * 1000) // 测量时间

#define AHT20_DEVICE_ADDR   0x38
#define AHT20_READ_ADDR     ((0x38<<1)|0x1)
#define AHT20_WRITE_ADDR    ((0x38<<1)|0x0)

#define AHT20_CMD_CALIBRATION       0xBE // 初始化（校准）命令
#define AHT20_CMD_CALIBRATION_ARG0  0x08
#define AHT20_CMD_CALIBRATION_ARG1  0x00

/**
 * 传感器在采集时需要时间,主机发出测量指令（0xAC）后，延时75毫秒以上再读取转换后的数据并判断返回的状态位是否正常。
 * 若状态比特位[Bit7]为0代表数据可正常读取，为1时传感器为忙状态，主机需要等待数据处理完成。
 **/
#define AHT20_CMD_TRIGGER       0xAC // 触发测量命令
#define AHT20_CMD_TRIGGER_ARG0  0x33
#define AHT20_CMD_TRIGGER_ARG1  0x00

// 用于在无需关闭和再次打开电源的情况下，重新启动传感器系统，软复位所需时间不超过20 毫秒
#define AHT20_CMD_RESET      0xBA // 软复位命令

#define AHT20_CMD_STATUS     0x71 // 获取状态命令

/**
 * STATUS 命令回复：
 * 1. 初始化后触发测量之前，STATUS 只回复 1B 状态值；
 * 2. 触发测量之后，STATUS 回复6B： 1B 状态值 + 2B 湿度 + 4b湿度 + 4b温度 + 2B 温度
 *      RH = Srh / 2^20 * 100%
 *      T  = St  / 2^20 * 200 - 50
 **/
#define AHT20_STATUS_BUSY_SHIFT 7       // bit[7] Busy indication
#define AHT20_STATUS_BUSY_MASK  (0x1<<AHT20_STATUS_BUSY_SHIFT)
// #define AHT20_STATUS_BUSY(status) (((status) & AHT20_STATUS_BUSY_MASK) >> AHT20_STATUS_BUSY_SHIFT)
uint8_t aht20_status_busy(uint8_t status)
{
    return (((status) & AHT20_STATUS_BUSY_MASK) >> AHT20_STATUS_BUSY_SHIFT);
}

#define AHT20_STATUS_MODE_SHIFT 5       // bit[6:5] Mode Status
#define AHT20_STATUS_MODE_MASK  (0x3<<AHT20_STATUS_MODE_SHIFT)
// #define AHT20_STATUS_MODE(status) (((status) & AHT20_STATUS_MODE_MASK) >> AHT20_STATUS_MODE_SHIFT)
uint8_t aht20_status_mode(uint8_t status)
{
    return (((status) & AHT20_STATUS_MODE_MASK) >> AHT20_STATUS_MODE_SHIFT);
}
                                        // bit[4] Reserved
#define AHT20_STATUS_CALI_SHIFT 3       // bit[3] CAL Enable
#define AHT20_STATUS_CALI_MASK  (0x1<<AHT20_STATUS_CALI_SHIFT)
// #define AHT20_STATUS_CALI(status) (((status) & AHT20_STATUS_CALI_MASK) >> AHT20_STATUS_CALI_SHIFT)
                                        // bit[2:0] Reserved
uint8_t aht20_status_cali(uint8_t status)
{
    return (((status) & AHT20_STATUS_CALI_MASK) >> AHT20_STATUS_CALI_SHIFT);
}

#define AHT20_STATUS_RESPONSE_MAX 6

#define AHT20_RESOLUTION            (1<<20)  // 2^20

#define AHT20_MAX_RETRY 10

#define SHIFT_4BIT  (4)
#define SHIFT_8BIT  (8)
#define NUM_50      (50)
#define NUM_100     (100)
#define NUM_200     (200)

static uint32_t AHT20_Read(uint8_t* buffer, uint32_t buffLen)
{
    hi_i2c_data data = { 0 };
    data.receive_buf = buffer;
    data.receive_len = buffLen;
    uint32_t retval = hi_i2c_read(AHT20_I2C_IDX, AHT20_READ_ADDR, &data);
    if (retval != HI_ERR_SUCCESS) {
        printf("I2cRead() failed, %0X!\n", retval);
        return retval;
    }
    return HI_ERR_SUCCESS;
}

static uint32_t AHT20_Write(uint8_t* buffer, uint32_t buffLen)
{
    hi_i2c_data data = { 0 };
    data.send_buf = buffer;
    data.send_len = buffLen;
    uint32_t retval = hi_i2c_write(AHT20_I2C_IDX, AHT20_WRITE_ADDR, &data);
    if (retval != HI_ERR_SUCCESS) {
        printf("I2cWrite(%02X) failed, %0X!\n", buffer[0], retval);
        return retval;
    }
    return HI_ERR_SUCCESS;
}

// 发送获取状态命令
static uint32_t AHT20_StatusCommand(void)
{
    uint8_t statusCmd[] = { AHT20_CMD_STATUS };
    return AHT20_Write(statusCmd, sizeof(statusCmd));
}

// 发送软复位命令
static uint32_t AHT20_ResetCommand(void)
{
    uint8_t resetCmd[] = {AHT20_CMD_RESET};
    return AHT20_Write(resetCmd, sizeof(resetCmd));
}

// 发送初始化校准命令
static uint32_t AHT20_CalibrateCommand(void)
{
    uint8_t clibrateCmd[] = {AHT20_CMD_CALIBRATION, AHT20_CMD_CALIBRATION_ARG0, AHT20_CMD_CALIBRATION_ARG1};
    return AHT20_Write(clibrateCmd, sizeof(clibrateCmd));
}

// 读取温湿度值之前， 首先要看状态字的校准使能位Bit[3]是否为 1(通过发送0x71可以获取一个字节的状态字)，
// 如果不为1，要发送0xBE命令(初始化)，此命令参数有两个字节， 第一个字节为0x08，第二个字节为0x00。
uint32_t AHT20_Calibrate(void)
{
    uint32_t retval = 0;
    uint8_t buffer[AHT20_STATUS_RESPONSE_MAX];
    // memset(&buffer, 0x0, sizeof(buffer));
    memset_s(&buffer, sizeof(buffer), 0x0, sizeof(buffer));

    retval = AHT20_StatusCommand();
    if (retval != HI_ERR_SUCCESS) {
        return retval;
    }

    retval = AHT20_Read(buffer, sizeof(buffer));
    if (retval != HI_ERR_SUCCESS) {
        return retval;
    }

 // if (AHT20_STATUS_BUSY(buffer[0]) || !AHT20_STATUS_CALI(buffer[0])) {
    if (aht20_status_busy(buffer[0]) || !aht20_status_cali(buffer[0])) {
        retval = AHT20_ResetCommand();
        if (retval != HI_ERR_SUCCESS) {
            return retval;
        }
        usleep(AHT20_STARTUP_TIME);
        retval = AHT20_CalibrateCommand();
        usleep(AHT20_CALIBRATION_TIME);
        return retval;
    }

    return HI_ERR_SUCCESS;
}

// 发送 触发测量 命令，开始测量
uint32_t AHT20_StartMeasure(void)
{
    uint8_t triggerCmd[] = {AHT20_CMD_TRIGGER, AHT20_CMD_TRIGGER_ARG0, AHT20_CMD_TRIGGER_ARG1};
    return AHT20_Write(triggerCmd, sizeof(triggerCmd));
}

#define IDX_0 (0)
#define IDX_1 (1)
#define IDX_2 (2)
#define IDX_3 (3)
#define IDX_4 (4)
#define IDX_5 (5)

// 接收测量结果，拼接转换为标准值
uint32_t AHT20_GetMeasureResult(float* temp, float* humi)
{
    uint32_t retval = 0, i = 0;
    if (temp == NULL || humi == NULL) {
        return HI_ERR_FAILURE;
    }

    uint8_t buffer[AHT20_STATUS_RESPONSE_MAX];
    // memset(&buffer, 0x0, sizeof(buffer));
    memset_s(&buffer, sizeof(buffer), 0x0, sizeof(buffer));
    retval = AHT20_Read(buffer, sizeof(buffer));  // recv status command result
    if (retval != HI_ERR_SUCCESS) {
        return retval;
    }

 // for (i = 0; AHT20_STATUS_BUSY(buffer[IDX_0]) && i < AHT20_MAX_RETRY; i++) {
    for (i = 0; aht20_status_busy(buffer[IDX_0]) && i < AHT20_MAX_RETRY; i++) {
        // printf("AHT20 device busy, retry %d/%d!\r\n", i, AHT20_MAX_RETRY);
        usleep(AHT20_MEASURE_TIME);
        retval = AHT20_Read(buffer, sizeof(buffer));  // recv status command result
        if (retval != HI_ERR_SUCCESS) {
            return retval;
        }
    }
    if (i >= AHT20_MAX_RETRY) {
        printf("AHT20 device always busy!\r\n");
        return HI_ERR_SUCCESS;
    }

    uint32_t humiRaw = buffer[IDX_1];
    humiRaw = (humiRaw << SHIFT_8BIT) | buffer[IDX_2];
    humiRaw = (humiRaw << SHIFT_4BIT) | ((buffer[IDX_3] & 0xF0) >> SHIFT_4BIT);
    *humi = humiRaw / (float)AHT20_RESOLUTION * NUM_100;

    uint32_t tempRaw = buffer[IDX_3] & 0x0F;
    tempRaw = (tempRaw << SHIFT_8BIT) | buffer[IDX_4];
    tempRaw = (tempRaw << SHIFT_8BIT) | buffer[IDX_5];
    *temp = tempRaw / (float)AHT20_RESOLUTION * NUM_200 - NUM_50;
    // printf("humi = %05X, %f, temp= %05X, %f\r\n", humiRaw, *humi, tempRaw, *temp);
    return HI_ERR_SUCCESS;
}

