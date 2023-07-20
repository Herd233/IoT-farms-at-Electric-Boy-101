#ifndef __BH1750_H
#define __BH1750_H
void BH1750_I2C_Init(void);
int I2C_WriteData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen);
int I2C_ReadData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen);
void BH1750_Init(void);
float BH1750_ReadLightIntensity(void);



#endif