#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "hi_i2c.h"
#include "iot_i2c.h"
#include "iot_i2c_ex.h"
#include "BH1750.h"

#define WRITE_BIT           0x00
#define READ_BIT            0x01

#define BH1750_SLAVE_ADDR   0x23 // 从机地址
#define BH1750_PWR_DOWN     0x00 // 关闭模块
#define BH1750_PWR_ON       0x01 // 打开模块等待测量指令
#define BH1750_RST          0x07 // 重置数据寄存器值在PowerOn模式下有效
#define BH1750_CON_H        0x10 // 连续高分辨率模式，1lx，120ms
#define BH1750_CON_H2       0x11 // 连续高分辨率模式，0.5lx，120ms
#define BH1750_CON_L        0x13 // 连续低分辨率模式，4lx，16ms
#define BH1750_ONE_H        0x20 // 一次高分辨率模式，1lx，120ms，测量后模块转到PowerDown模式
#define BH1750_ONE_H2       0x21 // 一次高分辨率模式，0.5lx，120ms，测量后模块转到PowerDown模式
#define BH1750_ONE_L        0x23 // 一次低分辨率模式，4lx，16ms，测量后模块转到PowerDown模式
#define IOT_I2C_IDX_0       0
#define IOT_I2C_IDX_1       1
#define IOT_I2C1_SDA_GPIO_0  0
#define IOT_I2C1_SCL_GPIO_1  1

/**
 @brief I2C驱动初始化
 @param 无
 @return 无
*/
void BH1750_I2C_Init(void)
{
    IoTGpioInit(IOT_I2C1_SDA_GPIO_0);
    IoTGpioInit(IOT_I2C1_SCL_GPIO_1);

    //GPIO_0复用为I2C1_SDA
    IoSetFunc(IOT_IO_NAME_GPIO_0, IOT_IO_FUNC_GPIO_0_I2C1_SDA);

    //GPIO_1复用为I2C1_SCL
    IoSetFunc(IOT_IO_NAME_GPIO_1, IOT_IO_FUNC_GPIO_1_I2C1_SCL);

    //baudrate: 400kbps
    IoTI2cInit(IOT_I2C_IDX_1, 400000);

    IoTI2cSetBaudrate(IOT_I2C_IDX_1,400000);
}

/**
 @brief I2C写数据函数
 @param slaveAddr -[in] 从设备地址
 @param regAddr -[in] 寄存器地址
 @param pData -[in] 写入数据
 @param dataLen -[in] 写入数据长度
 @return 错误码
*/
int I2C_WriteData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen)
{
    int ret;
    IotI2cData i2c_data = {0};

    if(0 != regAddr)
    {
        i2c_data.sendBuf = &regAddr;
        i2c_data.sendLen = 1;
        ret = hi_i2c_write(HI_I2C_IDX_1, (slaveAddr << 1) | WRITE_BIT, &i2c_data);
        if(ret != 0)
        {
            printf("===== Error: I2C write status1 = 0x%x! =====\r\n", ret);
            return 0;
        }
    }

    i2c_data.sendBuf = pData;
    i2c_data.sendLen = dataLen;
    ret = hi_i2c_write(HI_I2C_IDX_1, (slaveAddr << 1) | WRITE_BIT, &i2c_data);
    if(ret != 0)
    {
        printf("===== Error: I2C write status1 = 0x%x! =====\r\n", ret);
        return 0;
    }

    return 1;
}

/**
 @brief I2C读数据函数
 @param slaveAddr -[in] 从设备地址
 @param regAddr -[in] 寄存器地址
 @param pData -[in] 读出数据
 @param dataLen -[in] 读出数据长度
 @return 错误码
*/
int I2C_ReadData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen)
{
    int ret;
    IotI2cData i2c_data = {0};

    if(0 != regAddr)
    {
        i2c_data.sendBuf = &regAddr;
        i2c_data.sendLen = 1;
        ret = hi_i2c_write(HI_I2C_IDX_1, (slaveAddr << 1) | WRITE_BIT, &i2c_data);
        if(ret != 0)
        {
            printf("===== Error: I2C write status = 0x%x! =====\r\n", ret);
            return 0;
        }
    }

    i2c_data.receiveBuf = pData;
    i2c_data.receiveLen = dataLen;
    ret = hi_i2c_read(HI_I2C_IDX_1, (slaveAddr << 1) | READ_BIT, &i2c_data);
    if(ret != 0)
    {
        printf("===== Error: I2C read status = 0x%x! =====\r\n", ret);
        return 0;
    }

    return 1;
}

/**
 @brief BH1750初始化函数
 @param 无
 @return 无
*/
void BH1750_Init(void)
{
    uint8_t data;
    data = BH1750_PWR_ON;              // 发送启动命令
    I2C_WriteData(BH1750_SLAVE_ADDR, 0, &data, 1);
    data = BH1750_CON_H;               // 设置连续高分辨率模式，1lx，120ms
    I2C_WriteData(BH1750_SLAVE_ADDR, 0, &data, 1);
}

/**
 @brief BH1750获取光强度
 @param 无
 @return 光强度
*/
float BH1750_ReadLightIntensity(void)
{
    float lux = 0.0;
    uint8_t sensorData[2] = {0};
    I2C_ReadData(BH1750_SLAVE_ADDR, 0, sensorData, 2);
    lux = (sensorData[0] << 8 | sensorData[1]) / 1.2;
    return lux;
}




