#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f4xx_hal.h"

typedef struct {
    float ax, ay, az;   // gia toc, don vi g
    float gx, gy, gz;   // toc do goc, don vi do/giay (da tru offset)
} MPU6050_Data;

// Khoi tao cam bien. Tra ve HAL_OK neu thanh cong.
// *who nhan gia tri thanh ghi WHO_AM_I (MPU6050 that = 0x68).
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c, uint8_t *who);

// Do offset gyro: de xe dung yen, nam on dinh trong luc goi (~1 giay).
void MPU6050_CalibrateGyro(I2C_HandleTypeDef *hi2c, uint16_t samples);

// Doc 1 mau (14 byte trong 1 lan I2C).
HAL_StatusTypeDef MPU6050_Read(I2C_HandleTypeDef *hi2c, MPU6050_Data *d);

#endif
