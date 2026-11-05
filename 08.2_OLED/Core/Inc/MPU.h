/* MPU6050 驱动 · 头文件
 * 任务 4.2：读陀螺仪角速度与加速度，OLED 实时刷新
 *
 * 用 HAL 的 I2C 寄存器读写（Mem_Read / Mem_Write）实现
 */
#ifndef OLED2_MPU_H
#define OLED2_MPU_H

#include "main.h"

/* 7 位地址 0x68 左移一位 = 0xD0（HAL 需要 8 位地址） */
#define MPU_ADDR              0xD0

#define MPU_REG_SMPRT_DIV     0x19
#define MPU_REG_CONFIG        0x1A
#define MPU_REG_GYRO_CONFIG   0x1B
#define MPU_REG_ACCEL_CONFIG  0x1C
#define MPU_REG_INT_PIN_CFG   0x37
#define MPU_REG_INT_ENABLE    0x38
#define MPU_REG_ACCEL_XOUT_H  0x3B
#define MPU_REG_PWR_MGMT_1    0x6B
#define MPU_REG_WHO_AM_I      0x75

typedef struct
{
    int16_t acc_x_raw;
    int16_t acc_y_raw;
    int16_t acc_z_raw;
    int16_t temp_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;

    float acc_x;        /* 单位 g */
    float acc_y;
    float acc_z;
    float temp;         /* 摄氏度 */
    float gyro_x;       /* 单位 °/s */
    float gyro_y;
    float gyro_z;
} MPU_Data;

extern MPU_Data g_mpu;
extern uint8_t  g_mpu_ok;      /* 1 = 通信正常 */

void MPU_WriteReg(uint8_t reg, uint8_t val);
void MPU_ReadReg(uint8_t reg, uint8_t *val);
void MPU_ReadRegs(uint8_t reg, uint8_t len, uint8_t *buf);

void MPU_Init(void);
void MPU_ReadRaw(MPU_Data *m);
void MPU_Convert(MPU_Data *m);
void MPU_Update(MPU_Data *m);

#endif /* OLED2_MPU_H */
