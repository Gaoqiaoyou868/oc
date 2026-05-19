#include <reg51.h>

typedef unsigned int u16;
typedef unsigned char u8;

// 直接在串口中断里设置 P0，无动态扫描，8×8点阵上看到稳定的列
// P0.0=0 → 点阵第一列亮, P0.1=0 → 第二列亮

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
            if (cmd[0] == '1' && cmd[1] == '1') {
                P0 = 0xFE;   // P0.0=0 → 第一列亮
            } else if (cmd[0] == '1' && cmd[1] == '0') {
                P0 = 0xFF;   // 全灭
            } else if (cmd[0] == '2' && cmd[1] == '1') {
                P0 = 0xFD;   // P0.1=0 → 第二列亮
            } else if (cmd[0] == '2' && cmd[1] == '0') {
                P0 = 0xFF;   // 全灭
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

    TMOD &= 0x0F;
    TMOD |= 0x20;
    TH1 = 0xFD;  TL1 = 0xFD;
    TR1 = 1;

    SCON = 0x50;  PCON = 0x00;
    ES = 1;

    EA = 1;

    while (1);
}
