/* 0.96 寸 OLED（SSD1306 / I2C）· HAL 版驱动 */
#include "oled.h"
#include "oledfont.h"

/* I2C1 句柄在 main.c 里定义（CubeMX 生成），这里声明成 extern 才能用。
 * 如果以后把外设初始化拆到 i2c.c，改成 include "i2c.h" 即可。 */
extern I2C_HandleTypeDef hi2c1;

/* ======================= 第 1 层：I2C 传输（唯一碰 HAL 的地方） ======================= */

/* 写一个字节：cmd=OLED_CMD 时控制字节为 0x00，cmd=OLED_DATA 时为 0x40 */
void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    uint8_t buf[2];

    buf[0] = (cmd == OLED_DATA) ? 0x40 : 0x00; //命令
    buf[1] = dat;                              //数据

    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, buf, 2, 100);
}

/* ======================= 第 2 层：显存操作 ======================= */

/* 设置写入位置：page = y（0~7），x = 列（0~127） */
void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    OLED_WR_Byte(0xb0 + y, OLED_CMD);
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
    OLED_WR_Byte((x & 0x0f), OLED_CMD);
}

/* 开启显示 */
void OLED_Display_On(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);   /* 电荷泵设置命令 */
    OLED_WR_Byte(0x14, OLED_CMD);   /* 电荷泵 ON */
    OLED_WR_Byte(0xAF, OLED_CMD);   /* 开显示 */
}

/* 关闭显示 */
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);   /* 电荷泵 OFF */
    OLED_WR_Byte(0xAE, OLED_CMD);   /* 关显示 */
}

/* 清屏：整屏写 0（全黑） */
void OLED_Clear(void)
{
    uint8_t i, n;

    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for (n = 0; n < 128; n++)
        {
            OLED_WR_Byte(0, OLED_DATA);
        }
    }
}

/* 全屏点亮（测试用） */
void OLED_On(void)
{
    uint8_t i, n;

    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for (n = 0; n < 128; n++)
        {
            OLED_WR_Byte(0x01, OLED_DATA);
        }
    }
}

/* 用同一个字节填满全屏（0x00 全黑 / 0xFF 全亮 / 0xAA 斑马纹） */
void fill_picture(unsigned char fill_Data)
{
    unsigned char m, n;

    for (m = 0; m < 8; m++)
    {
        OLED_WR_Byte(0xb0 + m, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for (n = 0; n < 128; n++)
        {
            OLED_WR_Byte(fill_Data, OLED_DATA);
        }
    }
}

/* ======================= 第 3 层：字符与图形 ======================= */

/* 显示一个 ASCII 字符：x=0~127，y=页号 0~7，Char_Size 取 16 或 12 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    unsigned char c = 0, i = 0;

    c = chr - ' ';
    if (x > Max_Column - 1)
    {
        x = 0;
        y = y + 2;
    }

    if (Char_Size == 16)
    {
        OLED_Set_Pos(x, y);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
        OLED_Set_Pos(x, y + 1);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
    }
    else
    {
        OLED_Set_Pos(x, y);
        for (i = 0; i < 6; i++)
            OLED_WR_Byte(F6x8[c][i], OLED_DATA);
    }
}

/* 求 m 的 n 次方（ShowNum 用） */
static uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
        result *= m;
    return result;
}

/* 显示数字：num 为数值，len 为位数，size 为字号 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + (size / 2) * t, y, ' ', size);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }
        OLED_ShowChar(x + (size / 2) * t, y, temp + '0', size);
    }
}

/* 显示字符串 */
void OLED_ShowString(uint8_t x, uint8_t y, const char *chr, uint8_t Char_Size)
{
    unsigned char j = 0;

    while (chr[j] != '\0')
    {
        OLED_ShowChar(x, y, chr[j], Char_Size);
        x += 8;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
        j++;
    }
}

/* 显示汉字（需要 oledfont.h 里有 Hzk 字模） */
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no)
{
    uint8_t t;

    OLED_Set_Pos(x, y);
    for (t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[2 * no][t], OLED_DATA);

    OLED_Set_Pos(x, y + 1);
    for (t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[2 * no + 1][t], OLED_DATA);
}

/* 显示图片：x0~x1 列，y0~y1 页 */
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1,
                  unsigned char y1, const unsigned char BMP[])
{
    unsigned int j = 0;
    //。。？
    unsigned char x, y;

    for (y = y0; y < y1; y++)
    {
        OLED_Set_Pos(x0, y);//移动光标到X0,Y0.当然， 这里是一页一页刷的
        //YO,Y1,Y2...写页，
        for (x = x0; x < x1; x++)
        {//X0,X1,X2...写列
            OLED_WR_Byte(BMP[j++], OLED_DATA);//写数据
        }
    }
}

/* ======================= 初始化 ======================= */

/* 上电初始化序列 */
void OLED_Init(void)
{
    HAL_Delay(800);                 /* 上电等待，SSD1306 需要 */

    OLED_WR_Byte(0xAE, OLED_CMD);   /* 关显示 */
    OLED_WR_Byte(0x00, OLED_CMD);   /* 列低地址 */
    OLED_WR_Byte(0x10, OLED_CMD);   /* 列高地址 */
    OLED_WR_Byte(0x40, OLED_CMD);   /* 起始行 */
    OLED_WR_Byte(0xB0, OLED_CMD);   /* 页地址 */
    OLED_WR_Byte(0x81, OLED_CMD);   /* 对比度 */
    OLED_WR_Byte(0xFF, OLED_CMD);
    OLED_WR_Byte(0xA1, OLED_CMD);   /* 段重映射 */
    OLED_WR_Byte(0xA6, OLED_CMD);   /* 正常显示 */
    OLED_WR_Byte(0xA8, OLED_CMD);   /* 复用率 */
    OLED_WR_Byte(0x3F, OLED_CMD);
    OLED_WR_Byte(0xC8, OLED_CMD);   /* COM 扫描方向 */
    OLED_WR_Byte(0xD3, OLED_CMD);   /* 显示偏移 */
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0xD5, OLED_CMD);   /* 时钟分频 */
    OLED_WR_Byte(0x80, OLED_CMD);
    OLED_WR_Byte(0xD8, OLED_CMD);   /* 区域颜色模式 */
    OLED_WR_Byte(0x05, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);   /* 预充电周期 */
    OLED_WR_Byte(0xF1, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD);   /* COM 引脚配置 */
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);   /* VCOMH */
    OLED_WR_Byte(0x30, OLED_CMD);
    OLED_WR_Byte(0x8D, OLED_CMD);   /* 电荷泵使能 */
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);   /* 开显示 */
}
