/* MPU6050 HAL 驱动 · 头文件
 *
 * 来源：https://github.com/creative-apple/STM32-MPU6050
 * 改动：
 *   1. 去掉 #include "i2c.h"（本工程 hi2c1 定义在 main.c）
 *   2. 去掉 INT 引脚相关定义和 MPU6050_DataReady()（本工程不用数据就绪中断）
 *   3. 变量改名：MPU6050 -> g_mpu，新增 g_mpu_ok 状态标志
 *   4. 初始化里去掉 printf / while(1)，改成"失败就置标志返回"
 */
#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"

/* 7 位地址 0x68 左移一位 = 0xD0（HAL 需要 8 位地址） */
#define MPU6050_ADDR          0xD0

#define MPU6050_SMPRT_DIV     0x19
#define MPU6050_WHO_AM_I      0x75
#define MPU6050_CONFIG        0x1A
#define MPU6050_GYRO_CONFIG   0x1B
#define MPU6050_ACCEL_CONFIG  0x1C
#define MPU6050_INT_PIN_CFG   0x37
#define MPU6050_INT_ENABLE    0x38
#define MPU6050_INT_STATUS    0x3A
#define MPU6050_ACCEL_XOUT_H  0x3B
#define MPU6050_PWR_MGMT_1    0x6B

typedef struct _MPU6050
{
    int16_t acc_x_raw;
    int16_t acc_y_raw;
    int16_t acc_z_raw;
    int16_t temperature_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;

    float acc_x;        /* g */
    float acc_y;
    float acc_z;
    float temperature;  /* 摄氏度 */
    float gyro_x;       /* °/s */
    float gyro_y;
    float gyro_z;
} Struct_MPU6050;

extern Struct_MPU6050 g_mpu;    /* 六轴数据 */
extern uint8_t g_mpu_ok;        /* 1 = 通信正常，0 = 没接/失败 */

void MPU6050_Writebyte(uint8_t reg_addr, uint8_t val);
void MPU6050_Readbyte(uint8_t reg_addr, uint8_t *data);
void MPU6050_Readbytes(uint8_t reg_addr, uint8_t len, uint8_t *data);

void MPU6050_Initialization(void);
void MPU6050_Get6AxisRawData(Struct_MPU6050 *mpu);
void MPU6050_DataConvert(Struct_MPU6050 *mpu);
void MPU6050_ProcessData(Struct_MPU6050 *mpu);

#endif
