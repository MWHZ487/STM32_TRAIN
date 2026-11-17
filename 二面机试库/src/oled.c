/* 0.96 寸 OLED（SSD1306 / I2C）· HAL 版驱动 */
#include "oled.h"
#include "oledfont.h"
#include <string.h>

/* I2C1 句柄在 main.c 里定义（CubeMX 生成），这里声明成 extern 才能用。
 * 如果以后把外设初始化拆到 i2c.c，改成 include "i2c.h" 即可。 */
extern I2C_HandleTypeDef hi2c1;

/* 第一次传输失败后置 0，后面的写全部跳过，避免 OLED 没接好时每个字节都等超时 */
static uint8_t s_oled_ok = 1;

/* ======================= 第 1 层：I2C 传输（唯一碰 HAL 的地方） ======================= */

/* 一次 I2C 事务。失败只记一次，不再重试（重试会把主循环拖死） */
static void oled_tx(const uint8_t *buf, uint16_t len)
{
    if (!s_oled_ok)
        return;
    if (HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, (uint8_t *)buf, len, 20) != HAL_OK)
        s_oled_ok = 0;
}

/* 写一个字节：cmd=OLED_CMD 时控制字节为 0x00，cmd=OLED_DATA 时为 0x40 */
void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    uint8_t buf[2];

    buf[0] = (cmd == OLED_DATA) ? 0x40 : 0x00;
    buf[1] = dat;
    oled_tx(buf, 2);
}

/* 连续写数据：控制字节 0x40 后面跟一整段，一页最多 128 字节一次发完。
 * 比逐字节发送少几十倍的 I2C 事务（每次事务都要重新发地址）。 */
static void oled_write_data(const uint8_t *data, uint16_t len)
{
    uint8_t buf[129];

    while (len > 0 && s_oled_ok)
    {
        uint16_t n = (len > 128) ? 128 : len;

        buf[0] = 0x40;
        memcpy(buf + 1, data, n);
        oled_tx(buf, (uint16_t)(n + 1));
        data += n;
        len  -= n;
    }
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

/* 用同一个字节填满全屏（0x00 全黑 / 0xFF 全亮 / 0xAA 斑马纹） */
void fill_picture(unsigned char fill_Data)
{
    uint8_t line[128];
    uint8_t page;

    memset(line, fill_Data, sizeof line);
    for (page = 0; page < 8; page++)
    {
        OLED_Set_Pos(0, page);
        oled_write_data(line, 128);   /* 一页一次事务，清屏共 8 次而不是 1024 次 */
    }
}

/* 清屏：整屏写 0（全黑） */
void OLED_Clear(void)
{
    fill_picture(0x00);
}

/* 全屏点亮（测试用）。以前写 0x01，只会点亮每页最上面一行 */
void OLED_On(void)
{
    fill_picture(0xFF);
}

/* ======================= 第 3 层：字符与图形 ======================= */

/* 字库下标。超出 F8X16（95 个）或 F6x8（92 个）时返回 0xFF，调用方画空格 */
static uint8_t oled_glyph(uint8_t chr, uint8_t limit)
{
    if (chr < ' ' || (uint8_t)(chr - ' ') >= limit)
        return 0xFF;
    return (uint8_t)(chr - ' ');
}

/* 显示一个 ASCII 字符：x=0~127，y=页号 0~7，Char_Size 取 16 或 12 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    uint8_t c;
    uint8_t blank[8] = {0};

    if (x > Max_Column - 1)
    {
        x = 0;
        y = y + 2;
    }

    if (Char_Size == 16)
    {
        c = oled_glyph(chr, 95);          /* F8X16：' ' ~ '~' */
        OLED_Set_Pos(x, y);
        oled_write_data(c == 0xFF ? blank : &F8X16[c * 16], 8);
        OLED_Set_Pos(x, y + 1);
        oled_write_data(c == 0xFF ? blank : &F8X16[c * 16 + 8], 8);
    }
    else
    {
        c = oled_glyph(chr, 92);          /* F6x8 只有 92 个 */
        OLED_Set_Pos(x, y);
        oled_write_data(c == 0xFF ? blank : F6x8[c], 6);
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
    uint8_t cw = (size == 16) ? 8 : 6;    /* 字符步进要和 ShowString 一致 */

    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + cw * t, y, ' ', size);
                continue;
            }
            enshow = 1;
        }
        OLED_ShowChar(x + cw * t, y, temp + '0', size);
    }
}

/* 显示字符串 */
void OLED_ShowString(uint8_t x, uint8_t y, const char *chr, uint8_t Char_Size)
{
    uint8_t cw = (Char_Size == 16) ? 8 : 6;

    if (chr == NULL)
        return;

    while (*chr != '\0')
    {
        OLED_ShowChar(x, y, (uint8_t)*chr, Char_Size);
        x += cw;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
        chr++;
    }
}

/* 显示汉字（需要 oledfont.h 里有 Hzk 字模） */
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no)
{
    /* 一个字占两行，no 越界会读出字库 */
    if ((uint16_t)no * 2 + 1 >= sizeof(Hzk) / sizeof(Hzk[0]))
        return;

    OLED_Set_Pos(x, y);
    oled_write_data(Hzk[2 * no], 16);

    OLED_Set_Pos(x, y + 1);
    oled_write_data(Hzk[2 * no + 1], 16);
}

/* 显示图片：x0~x1 列，y0~y1 页。一行拼成一次传输 */
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1,
                  unsigned char y1, const unsigned char BMP[])
{
    unsigned int j = 0;
    unsigned char x, y;
    uint8_t row[128];

    for (y = y0; y < y1; y++)
    {
        unsigned char w;

        if (x1 <= x0)
            break;
        w = (unsigned char)(x1 - x0);
        if (w > 128)
            w = 128;
        for (x = 0; x < w; x++)
            row[x] = BMP[j++];
        OLED_Set_Pos(x0, y);
        oled_write_data(row, w);
    }
}

/* ======================= 初始化 ======================= */

/* 上电初始化序列 */
void OLED_Init(void)
{
    s_oled_ok = 1;                  /* 允许重新初始化时再试一次 */
    HAL_Delay(100);                 /* SSD1306 上电稳定，100ms 够用 */

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
