#include "mpu6050.h"

#define MPU_ADDR        (0x68 << 1)   // AD0 de trong/noi GND -> 0x68
#define REG_SMPLRT_DIV  0x19
#define REG_CONFIG      0x1A
#define REG_GYRO_CFG    0x1B
#define REG_ACCEL_CFG   0x1C
#define REG_ACCEL_XOUT  0x3B
#define REG_PWR_MGMT_1  0x6B
#define REG_WHO_AM_I    0x75

#define ACCEL_LSB_PER_G   16384.0f    // +-2 g
#define GYRO_LSB_PER_DPS  65.5f       // +-500 do/s

static float gx_off = 0, gy_off = 0, gz_off = 0;

static HAL_StatusTypeDef wr(I2C_HandleTypeDef *h, uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(h, MPU_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 20);
}

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *h, uint8_t *who)
{
    uint8_t id = 0;
    if (HAL_I2C_Mem_Read(h, MPU_ADDR, REG_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &id, 1, 20) != HAL_OK)
        return HAL_ERROR;
    if (who) *who = id;

    if (wr(h, REG_PWR_MGMT_1, 0x80) != HAL_OK) return HAL_ERROR;  // reset
    HAL_Delay(100);
    if (wr(h, REG_PWR_MGMT_1, 0x01) != HAL_OK) return HAL_ERROR;  // thuc day, clock = PLL gyro X
    HAL_Delay(10);
    wr(h, REG_SMPLRT_DIV, 0x00);   // 1 kHz (khi DLPF bat)
    wr(h, REG_CONFIG,     0x03);   // DLPF ~42-44 Hz, loc rung dong co
    wr(h, REG_GYRO_CFG,   0x08);   // +-500 do/s
    wr(h, REG_ACCEL_CFG,  0x00);   // +-2 g
    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_Read(I2C_HandleTypeDef *h, MPU6050_Data *d)
{
    uint8_t b[14];
    if (HAL_I2C_Mem_Read(h, MPU_ADDR, REG_ACCEL_XOUT, I2C_MEMADD_SIZE_8BIT, b, 14, 20) != HAL_OK)
        return HAL_ERROR;

    int16_t ax = (int16_t)((b[0]  << 8) | b[1]);
    int16_t ay = (int16_t)((b[2]  << 8) | b[3]);
    int16_t az = (int16_t)((b[4]  << 8) | b[5]);
    // b[6], b[7] = nhiet do (bo qua)
    int16_t gx = (int16_t)((b[8]  << 8) | b[9]);
    int16_t gy = (int16_t)((b[10] << 8) | b[11]);
    int16_t gz = (int16_t)((b[12] << 8) | b[13]);

    d->ax = ax / ACCEL_LSB_PER_G;
    d->ay = ay / ACCEL_LSB_PER_G;
    d->az = az / ACCEL_LSB_PER_G;
    d->gx = gx / GYRO_LSB_PER_DPS - gx_off;
    d->gy = gy / GYRO_LSB_PER_DPS - gy_off;
    d->gz = gz / GYRO_LSB_PER_DPS - gz_off;
    return HAL_OK;
}

void MPU6050_CalibrateGyro(I2C_HandleTypeDef *h, uint16_t samples)
{
    float sx = 0, sy = 0, sz = 0;
    uint16_t n = 0;
    MPU6050_Data d;
    gx_off = gy_off = gz_off = 0;
    for (uint16_t i = 0; i < samples; i++) {
        if (MPU6050_Read(h, &d) == HAL_OK) {
            sx += d.gx; sy += d.gy; sz += d.gz; n++;
        }
        HAL_Delay(2);
    }
    if (n) { gx_off = sx / n; gy_off = sy / n; gz_off = sz / n; }
}
