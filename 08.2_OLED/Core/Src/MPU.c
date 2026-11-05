/* MPU6050 驱动 · 源文件*/
#include "MPU.h"

extern I2C_HandleTypeDef hi2c1;

MPU_Data g_mpu;
uint8_t  g_mpu_ok = 0;

/* 灵敏度（量程决定） */
static float s_lsb_acc  = 16384.0f;   /* ±2g      */
static float s_lsb_gyro = 131.0f;     /* ±250°/s  */

/* ---------------- 底层寄存器读写 ---------------- */

void MPU_WriteReg(uint8_t reg, uint8_t val)  
{
    HAL_I2C_Mem_Write(&hi2c1, MPU_ADDR, reg,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

void MPU_ReadReg(uint8_t reg, uint8_t *val)
{
    HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR, reg,
                     I2C_MEMADD_SIZE_8BIT, val, 1, 100);
}

void MPU_ReadRegs(uint8_t reg, uint8_t len, uint8_t *buf)
{
    HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR, reg,
                     I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

/* ---------------- 初始化 ---------------- */

void MPU_Init(void)
{
    uint8_t id = 0;

    g_mpu_ok = 0;
    HAL_Delay(50);

    /* ① 身份验证（睡眠状态下也能读） */
    MPU_ReadReg(MPU_REG_WHO_AM_I, &id);
    if (id != 0x68)
    {
        return;
    }

    /* ② 复位整个模块 */
    MPU_WriteReg(MPU_REG_PWR_MGMT_1, 0x80);
    HAL_Delay(100);

    /* ③ 唤醒：清 SLEEP 位（bit6） */
    MPU_WriteReg(MPU_REG_PWR_MGMT_1, 0x00);
    HAL_Delay(50);

    /* ④ 采样率：1kHz / (1 + 39) = 200Hz */
    MPU_WriteReg(MPU_REG_SMPRT_DIV, 39);
    HAL_Delay(20);

    /* ⑤ 数字低通滤波：0（带宽最大） */
    MPU_WriteReg(MPU_REG_CONFIG, 0x00);
    HAL_Delay(20);

    /* ⑥ 陀螺量程 ±250°/s（FS_SEL = 0） */
    MPU_WriteReg(MPU_REG_GYRO_CONFIG, 0x00);
    HAL_Delay(20);

    /* ⑦ 加速度量程 ±2g（AFS_SEL = 0） */
    MPU_WriteReg(MPU_REG_ACCEL_CONFIG, 0x00);
    HAL_Delay(20);

    s_lsb_gyro = 131.0f;
    s_lsb_acc  = 16384.0f;

    g_mpu_ok = 1;
}

/* ---------------- 读取与换算 ---------------- */

void MPU_ReadRaw(MPU_Data *m)  //指针指向数据，这里就是读就好了，可以用指针指向结构体内部的东西，，，所以这个貌似就是用来读外设MPU的 
{
    uint8_t buf[14];  //十四位数组

    MPU_ReadRegs(MPU_REG_ACCEL_XOUT_H, 14, buf); //读取高位的数据

    m->acc_x_raw = (int16_t)((buf[0] << 8) | buf[1]);  //高位左移8位，低位直接加上去，拼成真实数据
    m->acc_y_raw = (int16_t)((buf[2] << 8) | buf[3]);
    m->acc_z_raw = (int16_t)((buf[4] << 8) | buf[5]);

    m->temp_raw  = (int16_t)((buf[6] << 8) | buf[7]);

    m->gyro_x_raw = (int16_t)((buf[8]  << 8) | buf[9]);
    m->gyro_y_raw = (int16_t)((buf[10] << 8) | buf[11]);
    m->gyro_z_raw = (int16_t)((buf[12] << 8) | buf[13]);
}

void MPU_Convert(MPU_Data *m)
{
    m->acc_x = (float)m->acc_x_raw / s_lsb_acc;
    m->acc_y = (float)m->acc_y_raw / s_lsb_acc;
    m->acc_z = (float)m->acc_z_raw / s_lsb_acc;

    /* 温度：raw / 340 + 36.53 */
    m->temp = (float)m->temp_raw / 340.0f + 36.53f;

    m->gyro_x = (float)m->gyro_x_raw / s_lsb_gyro;
    m->gyro_y = (float)m->gyro_y_raw / s_lsb_gyro;
    m->gyro_z = (float)m->gyro_z_raw / s_lsb_gyro;
}

void MPU_Update(MPU_Data *m)
{
    MPU_ReadRaw(m);
    MPU_Convert(m);
}
