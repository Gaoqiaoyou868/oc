/******************************************************************************
 * LCD1602 测试 v2 — 保留段选跳线帽, 避开数码管干扰
 *
 * 关键改动: 段选跳线帽 ✅ 插上 (给 P0 提供上拉电阻)
 *           位选跳线帽 ❌ 拔掉 (断开 138 译码器, 数码管不亮)
 *           P2.2~P2.4 全部置高, 让 138 选中不存在的位
 *============================================================================*/

#include <reg51.h>
#include <intrins.h>

#define LCD_DATA P0
sbit LCD_RS = P2^0;
sbit LCD_RW = P2^1;
sbit LCD_EN = P2^2;

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

    Delay_ms(50);                  /* 等电源稳定 */

    /* 软复位 (按 HD44780 数据手册的 8 位初始化流程) */
    for (i = 0; i < 3; i++) {
        LCD_DATA = 0x30;
        LCD_RS = 0;
        LCD_RW = 0;
        LCD_Pulse();
        Delay_ms(6);
    }

    LCD_WriteCmd(0x38);            /* 8位, 2行, 5x8 */
    LCD_WriteCmd(0x08);            /* 关显示 */
    LCD_WriteCmd(0x01);            /* 清屏 */
    Delay_ms(5);
    LCD_WriteCmd(0x06);            /* 光标右移 */
    LCD_WriteCmd(0x0C);            /* 开显示, 关光标 */
}

void main(void)
{
    unsigned char count = 0;

    /* 端口初始化 */
    P0 = 0xFF;                     /* LCD 数据口 */
    P1 = 0xFF;                     /* 全高 */
    P2 = 0xFF;                     /* 全高, P2.2~P2.4=高 → 138选Y7(空) */
    P3 = 0xFF;                     /* 全高 */

    LCD_Init();

    /* 第一行 */
    LCD_SetPos(0, 0);
    LCD_WriteStr("LCD OK!         ");

    /* 第二行: 动态显示计数值, 证明 MCU 在运行 */
    LCD_SetPos(1, 0);
    LCD_WriteStr("Count: 000      ");

    while (1) {
        /* 更新第二行的计数器 */
        LCD_SetPos(1, 7);
        LCD_WriteData('0' + (count / 100) % 10);
        LCD_WriteData('0' + (count / 10)  % 10);
        LCD_WriteData('0' + (count)       % 10);

        count++;
        if (count >= 256) count = 0;

        Delay_ms(500);             /* 每 0.5 秒更新一次 */
    }
}
