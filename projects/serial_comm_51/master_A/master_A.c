#include <reg51.h>

typedef unsigned int u16;
typedef unsigned char u8;

sbit KEY1 = P3^2;
sbit KEY2 = P3^3;
// 普中A2: 7SEG经74HC138驱动 (P1.0=A, P1.1=B, P1.2=C)
// Y0=十位 (P1=0x00), Y1=个位 (P1=0x01)

u8 code seg_CA[10] = {
    0xC0, 0xF9, 0xA4, 0xB0,
    0x99, 0x92, 0x82, 0xF8,
    0x80, 0x90
};

u8 disp_val = 0;

void Delay_ms(u16 ms)
{
    u16 i, j;
    for (i = ms; i > 0; i--)
        for (j = 120; j > 0; j--);
}

void UART_SendByte(u8 dat)
{
    SBUF = dat;
    while (!TI);
    TI = 0;
}

void INT0_ISR(void) interrupt 0
{
    Delay_ms(20);
    if (KEY1 == 0) {
        UART_SendByte(0x11);
        while (KEY1 == 0);
        Delay_ms(20);
    }
}

void INT1_ISR(void) interrupt 2
{
    Delay_ms(20);
    if (KEY2 == 0) {
        UART_SendByte(0x22);
        while (KEY2 == 0);
        Delay_ms(20);
    }
}

void Timer0_ISR(void) interrupt 1
{
    static u8 scan = 0;

    TH0 = 0xF8;  TL0 = 0xCD;

    P0 = 0xFF;                // 消隐
    P1 = (P1 & 0xF8);         // 关位选 (138: A/B/C=0)

    scan = !scan;
    if (scan) {
        P0 = seg_CA[disp_val / 10];
        P1 = (P1 & 0xF8) | 0x00;  // 138: Y0 → 十位
    } else {
        P0 = seg_CA[disp_val % 10];
        P1 = (P1 & 0xF8) | 0x01;  // 138: Y1 → 个位
    }
}

void UART_ISR(void) interrupt 4
{
    if (RI) {
        disp_val = SBUF;
        RI = 0;
    }
}

void main(void)
{
    P0 = 0xFF;
    P1 = 0x00;
    P2 = 0xFF;
    P3 = 0xFF;

    disp_val = 0;

    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = 0xF8;  TL0 = 0xCD;
    ET0 = 1;  TR0 = 1;

    TMOD &= 0x0F;
    TMOD |= 0x20;
    TH1 = 0xFD;  TL1 = 0xFD;
    TR1 = 1;

    SCON = 0x50;  PCON = 0x00;
    ES = 1;

    IT0 = 1;  EX0 = 1;
    IT1 = 1;  EX1 = 1;

    PX0 = 1;  PX1 = 1;
    PT0 = 0;  PS = 0;

    EA = 1;

    while (1);
}
