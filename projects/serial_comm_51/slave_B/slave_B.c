#include <reg51.h>

typedef unsigned int u16;
typedef unsigned char u8;

sbit K1 = P3^2;

// 选做(1): K1(P3.2)按下 → 发送 "Hello 齐继浩 094424128"
// 选做(2): 串口发"11"/"10"/"21"/"20" → 控制 D1/D2

u8 code hello_msg[] = "Hello 齐继浩 094424128\r\n";

void Delay_ms(u16 ms)
{
    u16 i, j;
    for (i = ms; i > 0; i--)
        for (j = 120; j > 0; j--);
}

void UART_SendByte(u8 dat)
{
    ES = 0;
    SBUF = dat;
    while (!TI);
    TI = 0;
    ES = 1;
}

void UART_SendString(void)
{
    u8 i;
    for (i = 0; hello_msg[i] != '\0'; i++) {
        UART_SendByte(hello_msg[i]);
    }
}

void INT0_ISR(void) interrupt 0
{
    Delay_ms(20);
    if (K1 == 0) {
        UART_SendString();
        while (K1 == 0);
        Delay_ms(20);
    }
}

void UART_ISR(void) interrupt 4
{
    static u8 cmd[3];
    static u8 idx = 0;
    u8 ch;

    if (RI) {
        ch = SBUF;
        RI = 0;

        if (ch == '1' || ch == '2') {
            cmd[idx++] = ch;
        } else if (ch == '0' && idx > 0) {
            cmd[idx++] = ch;
        } else {
            idx = 0;
            return;
        }

        if (idx == 2) {
            if (cmd[0] == '1' && cmd[1] == '1')
                P0 &= 0xFE;
            else if (cmd[0] == '1' && cmd[1] == '0')
                P0 |= 0x01;
            else if (cmd[0] == '2' && cmd[1] == '1')
                P0 &= 0xFD;
            else if (cmd[0] == '2' && cmd[1] == '0')
                P0 |= 0x02;
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

    TMOD &= 0x0F;
    TMOD |= 0x20;
    TH1 = 0xFD;  TL1 = 0xFD;
    TR1 = 1;

    SCON = 0x50;  PCON = 0x00;
    TI = 0;
    RI = 0;

    IT0 = 1;
    EX0 = 1;

    ES = 1;
    EA = 1;

    while (1);
}
