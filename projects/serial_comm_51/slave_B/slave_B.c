#include <reg51.h>

typedef unsigned int u16;
typedef unsigned char u8;

u8 code seg_CA[10] = {
    0xC0, 0xF9, 0xA4, 0xB0,
    0x99, 0x92, 0x82, 0xF8,
    0x80, 0x90
};

u8 disp_buf[2] = {0xFF, 0xFF};

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

void Timer0_ISR(void) interrupt 1
{
    static u8 scan = 0;

    TH0 = 0xF8;  TL0 = 0xCD;

    P0 = 0xFF;
    P1 = (P1 & 0xF8);

    if (scan == 0) {
        if (disp_buf[0] != 0xFF) {
            P0 = disp_buf[0];
            P1 = (P1 & 0xF8) | 0x00;
        }
    } else {
        if (disp_buf[1] != 0xFF) {
            P0 = disp_buf[1];
            P1 = (P1 & 0xF8) | 0x01;
        }
    }

    scan = !scan;
}

void UART_ISR(void) interrupt 4
{
    static u8 cmd[3];
    static u8 idx = 0;
    u8 ch;

    if (RI) {
        ch = SBUF;
        RI = 0;

        if (ch == '1') {
            cmd[idx++] = ch;
        } else if (ch == '2') {
            cmd[idx++] = ch;
        } else if (ch == '0' && idx > 0) {
            cmd[idx++] = ch;
        } else {
            idx = 0;
            return;
        }

        if (idx == 2) {
            UART_SendByte(cmd[0]);
            UART_SendByte(cmd[1]);

            if (cmd[0] == '1' && cmd[1] == '1') {
                disp_buf[0] = seg_CA[1];
                disp_buf[1] = seg_CA[1];
            } else if (cmd[0] == '1' && cmd[1] == '0') {
                disp_buf[0] = seg_CA[1];
                disp_buf[1] = seg_CA[0];
            } else if (cmd[0] == '2' && cmd[1] == '1') {
                disp_buf[0] = seg_CA[2];
                disp_buf[1] = seg_CA[1];
            } else if (cmd[0] == '2' && cmd[1] == '0') {
                disp_buf[0] = seg_CA[2];
                disp_buf[1] = seg_CA[0];
            }

            idx = 0;
        }

        if (idx >= 3) idx = 0;
    }
}

void main(void)
{
    P0 = 0xFF;
    P1 = 0x00;
    P2 = 0xFF;
    P3 = 0xFF;

    disp_buf[0] = 0xFF;
    disp_buf[1] = 0xFF;

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

    EA = 1;

    while (1);
}
