/* MPU6050 HAL 驱动 · 源文件
 *
 * 来源：https://github.com/creative-apple/STM32-MPU6050 （HAL 版）
 * 改动见头文件顶部注释：
 *   - 去 printf / while(1)，失败置 g_mpu_ok = 0 返回
 *   - 去 INT 引脚相关
 *   - 变量改为 g_mpu / g_mpu_ok
 *   - 超时统一 100ms（原库是 1ms，用杜邦线时容易超时）
 */
#include "mpu6050.h"

extern I2C_HandleTypeDef hi2c1;

Struct_MPU6050 g_mpu;
uint8_t g_mpu_ok = 0;

static float LSB_Sensitivity_ACC  = 16384.0f;   /* ±2g */
static float LSB_Sensitivity_GYRO = 131.0f;     /* ±250°/s */

/* ---------------- 底层读写 ---------------- */

void MPU6050_Writebyte(uint8_t reg_addr, uint8_t val)
{
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, reg_addr,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

void MPU6050_Readbyte(uint8_t reg_addr, uint8_t *data)
{
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, reg_addr,
                     I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

void MPU6050_Readbytes(uint8_t reg_addr, uint8_t len, uint8_t *data)
{
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, reg_addr,
                     I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

/* ---------------- 初始化 ---------------- */

void MPU6050_Initialization(void)
{
    uint8_t who_am_i = 0;

    g_mpu_ok = 0;
    HAL_Delay(50);

    /* ① 点名：确认芯片在线且型号正确 */
    MPU6050_Readbyte(MPU6050_WHO_AM_I, &who_am_i);
    if (who_am_i != 0x68)
    {
        return;                     /* 没接/接线错，直接返回，死循环改成返回 */
    }

    /* ② 复位整个模块 */
    MPU6050_Writebyte(MPU6050_PWR_MGMT_1, 0x80);
    HAL_Delay(100);

    /* ③ 唤醒（清 SLEEP 位） */
    MPU6050_Writebyte(MPU6050_PWR_MGMT_1, 0x00);
    HAL_Delay(50);

    /* ④ 采样率：1kHz / (1 + 39) = 200Hz */
    MPU6050_Writebyte(MPU6050_SMPRT_DIV, 39);
    HAL_Delay(20);

    /* ⑤ 数字低通滤波 0（带宽最大） */
    MPU6050_Writebyte(MPU6050_CONFIG, 0x00);
    HAL_Delay(20);

    /* ⑥ 陀螺量程 ±250°/s（FS_SEL = 0） */
    MPU6050_Writebyte(MPU6050_GYRO_CONFIG, (uint8_t)(0x00 << 3));
    HAL_Delay(20);

    /* ⑦ 加速度量程 ±2g（AFS_SEL = 0） */
    MPU6050_Writebyte(MPU6050_ACCEL_CONFIG, (uint8_t)(0x00 << 3));
    HAL_Delay(20);

    /* ⑧ 数据就绪中断配置（本工程不接 INT 引脚，配了也不影响） */
    MPU6050_Writebyte(MPU6050_INT_PIN_CFG, 0x10);
    HAL_Delay(20);
    MPU6050_Writebyte(MPU6050_INT_ENABLE, 0x01);
    HAL_Delay(20);

    LSB_Sensitivity_GYRO = 131.0f;      /* ±250°/s */
    LSB_Sensitivity_ACC  = 16384.0f;    /* ±2g */

    g_mpu_ok = 1;
}

/* ---------------- 读数与换算 ---------------- */

void MPU6050_Get6AxisRawData(Struct_MPU6050 *mpu)
{
    uint8_t data[14];

    MPU6050_Readbytes(MPU6050_ACCEL_XOUT_H, 14, data);

    mpu->acc_x_raw = (int16_t)((data[0] << 8) | data[1]);
    mpu->acc_y_raw = (int16_t)((data[2] << 8) | data[3]);
    mpu->acc_z_raw = (int16_t)((data[4] << 8) | data[5]);

    mpu->temperature_raw = (int16_t)((data[6] << 8) | data[7]);

    mpu->gyro_x_raw = (int16_t)((data[8]  << 8) | data[9]);
    mpu->gyro_y_raw = (int16_t)((data[10] << 8) | data[11]);
    mpu->gyro_z_raw = (int16_t)((data[12] << 8) | data[13]);
}

void MPU6050_DataConvert(Struct_MPU6050 *mpu)
{
    mpu->acc_x = (float)mpu->acc_x_raw / LSB_Sensitivity_ACC;
    mpu->acc_y = (float)mpu->acc_y_raw / LSB_Sensitivity_ACC;
    mpu->acc_z = (float)mpu->acc_z_raw / LSB_Sensitivity_ACC;

    /* 温度换算：raw / 340 + 36.53 */
    mpu->temperature = (float)mpu->temperature_raw / 340.0f + 36.53f;

    mpu->gyro_x = (float)mpu->gyro_x_raw / LSB_Sensitivity_GYRO;
    mpu->gyro_y = (float)mpu->gyro_y_raw / LSB_Sensitivity_GYRO;
    mpu->gyro_z = (float)mpu->gyro_z_raw / LSB_Sensitivity_GYRO;
}

void MPU6050_ProcessData(Struct_MPU6050 *mpu)
{
    MPU6050_Get6AxisRawData(mpu);
    MPU6050_DataConvert(mpu);
}
