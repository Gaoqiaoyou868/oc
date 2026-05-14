/******************************************************************************
 * LCD1602 测试 v3 — 使用普中A2标准引脚映射
 *
 * RS=P2.6, RW=P2.5, EN=P2.7, D0~D7=P0
 * 这才是普中A2板的正确LCD引脚定义!
 *============================================================================*/

#include <reg51.h>
#include <intrins.h>

#define LCD_DATA P0
sbit LCD_RS = P2^6;
sbit LCD_RW = P2^5;
sbit LCD_EN = P2^7;

void Delay_ms(unsigned int ms)
{
    unsigned int i;
    while (ms--) {
        for (i = 0; i < 120; i++) { _nop_(); }
    }
}

void Delay_us(unsigned int us)
{
    unsigned int i;
    for (i = 0; i < us; i++) { _nop_(); }
}

void LCD_Pulse(void)
{
    LCD_EN = 1;
    _nop_(); _nop_();
    LCD_EN = 0;
}

void LCD_WriteCmd(unsigned char cmd)
{
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_DATA = cmd;
    LCD_Pulse();
    Delay_us(60);
}

void LCD_WriteData(unsigned char dat)
{
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_DATA = dat;
    LCD_Pulse();
    Delay_us(50);
}

void LCD_SetPos(unsigned char line, unsigned char col)
{
    LCD_WriteCmd((line == 0 ? 0x80 : 0xC0) + col);
}

void LCD_WriteStr(unsigned char *str)
{
    while (*str) {
        LCD_WriteData(*str++);
    }
}

void LCD_Init(void)
{
    unsigned char i;
    Delay_ms(50);

    for (i = 0; i < 3; i++) {
        LCD_DATA = 0x30;
        LCD_RS = 0; LCD_RW = 0;
        LCD_Pulse();
        Delay_ms(6);
    }

    LCD_WriteCmd(0x38);
    LCD_WriteCmd(0x08);
    LCD_WriteCmd(0x01);
    Delay_ms(5);
    LCD_WriteCmd(0x06);
    LCD_WriteCmd(0x0C);
}

void main(void)
{
    unsigned char count = 0;

    P0 = 0xFF;
    P1 = 0xFF;
    P2 = 0xFF;
    P3 = 0xFF;

    LCD_Init();

    LCD_SetPos(0, 0);
    LCD_WriteStr("LCD OK! P26 P27 ");

    LCD_SetPos(1, 0);
    LCD_WriteStr("Count: 000      ");

    while (1) {
        LCD_SetPos(1, 7);
        LCD_WriteData('0' + (count / 100) % 10);
        LCD_WriteData('0' + (count / 10)  % 10);
        LCD_WriteData('0' + (count)       % 10);

        count++;
        if (count >= 256) count = 0;

        Delay_ms(500);
    }
}
