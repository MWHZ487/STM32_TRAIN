/* 0.96 寸 OLED（SSD1306 / I2C）· HAL 版驱动头文件*/
#ifndef __OLED_H
#define __OLED_H

#include "main.h"
#include <stdint.h>

/* 7 位地址 0x3C 左移一位 = 0x78（HAL 接口要求传 8 位地址） */
#define OLED_ADDR   0x78

#define OLED_CMD    0
#define OLED_DATA   1

#define Max_Column  128
#define Max_Row      64
#define X_WIDTH     128
#define Y_WIDTH      64
#define Brightness  0xFF

void OLED_WR_Byte(uint8_t dat, uint8_t cmd);
void OLED_Init(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Clear(void);
void OLED_On(void);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, const char *p, uint8_t Char_Size);
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no);
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1,
                  unsigned char y1, const unsigned char BMP[]);
void fill_picture(unsigned char fill_Data);

#endif
