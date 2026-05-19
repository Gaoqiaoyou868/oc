#include <reg51.h>

typedef unsigned int u16;
typedef unsigned char u8;

sbit KEY3 = P3^2;
sbit LED1 = P2^6;
sbit LED2 = P2^7;

u8 key_count = 0;
u8 rx_buf[4];
u8 rx_idx = 0;

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
    if (KEY3 == 0) {
        key_count++;
        if (key_count > 99) key_count = 0;
        UART_SendByte(key_count);
        while (KEY3 == 0);
        Delay_ms(20);
    }
}

void UART_ISR(void) interrupt 4
{
    u8 ch;

    if (RI) {
        ch = SBUF;
        RI = 0;

        if (ch == 0x11) {
            LED1 = 0;
            LED2 = 1;
            rx_idx = 0;
            return;
        }
        if (ch == 0x22) {
            LED1 = 1;
            LED2 = 1;
            rx_idx = 0;
            return;
        }

        if (ch >= '0' && ch <= '9') {
            if (rx_idx < 3)
                rx_buf[rx_idx++] = ch;
        } else {
            rx_idx = 0;
            return;
        }

        if (rx_idx == 2) {
            if (rx_buf[0] == '1' && rx_buf[1] == '1')
                LED1 = 0;
            else if (rx_buf[0] == '1' && rx_buf[1] == '0')
                LED1 = 1;
            else if (rx_buf[0] == '2' && rx_buf[1] == '1')
                LED2 = 0;
            else if (rx_buf[0] == '2' && rx_buf[1] == '0')
                LED2 = 1;
            rx_idx = 0;
        }

        if (rx_idx >= 3) rx_idx = 0;
    }
}

void main(void)
{
    P2 = 0xFF;
    key_count = 0;
    rx_idx = 0;

    TMOD &= 0x0F;
    TMOD |= 0x20;
    TH1 = 0xFD;  TL1 = 0xFD;
    TR1 = 1;

    SCON = 0x50;  PCON = 0x00;
    ES = 1;

    IT0 = 1;  EX0 = 1;

    PX0 = 1;
    PS = 0;

    EA = 1;

    while (1);
}
