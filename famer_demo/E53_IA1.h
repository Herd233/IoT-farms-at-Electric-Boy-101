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

#ifndef __E53_IA1_H__
#define __E53_IA1_H__

/* 寄存器宏定义 --------------------------------------------------------------------*/
#define BH1750_ADDR 0x23
#define BH1750_SEND_DATA_LEN 2
#define BH1750_RECV_DATA_LEN 2
#define BH1750_READ_DELAY_US 180000
#define BH1750_COEFFICIENT_LUX 1.2

#define SHT30_ADDR 0x44
#define SHT30_SEND_DATA_LEN 2
#define SHT30_TEMP_DATA_LEN 2
#define SHT30_HUM_DATA_LEN 2
#define SHT30_CONSTANT_1          175
#define SHT30_CONSTANT_2          45
#define SHT30_CONSTANT_3          100

#define DATA_WIDTH_8_BIT         8 // 8 bit

#define WIFI_IOT_IO_FUNC_GPIO_0_I2C1_SDA 6
#define WIFI_IOT_IO_FUNC_GPIO_1_I2C1_SCL 6
#define WIFI_IOT_IO_FUNC_GPIO_8_GPIO 0
#define WIFI_IOT_IO_FUNC_GPIO_14_GPIO 4
#define WIFI_IOT_I2C_IDX_1 1
#define WIFI_IOT_I2C_BAUDRATE 400000

#define E53_IA1_LIGHT_GPIO 14
#define E53_IA1_MOTOR_GPIO 8
#define E53_IA1_I2C1_SDA_GPIO 0
#define E53_IA1_I2C1_SCL_GPIO 1

typedef enum {
    OFF = 0,
    ON
} E53IA1Status;

/* E53_IA1传感器数据类型定义 ------------------------------------------------------------*/
typedef struct {
    float    Lux;			  // 光照强度
    float    Humidity;        // 湿度
    float    Temperature;     // 温度
} E53IA1Data;

enum CheckDataPart {
    DATA_LSB = 0,
    DATA_MSB = 1,
    DATA_CHECK = 2,
    CHECK_DATA_BUTT,
};

enum Sht3xDataBufferPart {
    TEMP_LSB = 0,
    TEMP_MSB = 1,
    TEMP_HSB = 2,
    HUM_LSB = 3,
    HUM_MSB = 4,
    HUM_HSB = 5,
    SHT3X_DATA_BUTT,
};

int E53IA1Init(void);
int E53IA1ReadData(E53IA1Data *ReadData);
void LightStatusSet(E53IA1Status status);
void MotorStatusSet(E53IA1Status status);


#endif /* __E53_IA1_H__ */

