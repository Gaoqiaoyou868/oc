#include <reg51.h>

typedef unsigned int u16;
typedef unsigned char u8;

// 普中A2: K1=RESET键（不是GPIO），按重启后自动发字符串
// D1=P0.0, D2=P0.1（低电平亮）
// 8×8点阵共用P0总线，列会跟着亮，属正常现象

u8 code hello_msg[] = "Hello 齐继浩 094424128\r\n";

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

void UART_SendString(void)
{
    u8 i;
    for (i = 0; hello_msg[i] != '\0'; i++) {
        UART_SendByte(hello_msg[i]);
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
                P0 &= 0xFE;      // D1亮 (P0.0=0)
            else if (cmd[0] == '1' && cmd[1] == '0')
                P0 |= 0x01;      // D1灭 (P0.0=1)
            else if (cmd[0] == '2' && cmd[1] == '1')
                P0 &= 0xFD;      // D2亮 (P0.1=0)
            else if (cmd[0] == '2' && cmd[1] == '0')
                P0 |= 0x02;      // D2灭 (P0.1=1)
            idx = 0;
        }

        if (idx >= 3) idx = 0;
    }

    if (TI) {
        TI = 0;
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

    // 选做(1): K1=RESET, 按重启 → 自动发消息
    UART_SendString();

    ES = 1;
    EA = 1;

    while (1);
}
